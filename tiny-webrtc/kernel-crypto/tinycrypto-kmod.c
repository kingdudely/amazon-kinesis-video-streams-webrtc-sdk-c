#include <linux/crypto.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <crypto/aead.h>
#include <crypto/hash.h>
#include "include/tinycrypto_uapi.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Tiny WebRTC kernel crypto provider");

#define TINYCRYPTO_MAX_INPUT (1U << 20)
#define TINYCRYPTO_MAX_AAD   (1U << 16)

static struct crypto_shash *sha1_tfm;
static struct crypto_shash *sha256_tfm;
static struct crypto_shash *hmac_sha1_tfm;
static struct crypto_shash *hmac_sha256_tfm;
static struct crypto_aead *gcm_tfm;
static DEFINE_MUTEX(crypto_lock);

static int tiny_hash(struct crypto_shash *tfm, const void __user *input, u32 input_len, void *output)
{
    struct shash_desc *desc;
    u8 *buf;
    int ret;

    if (!tfm || input_len > TINYCRYPTO_MAX_INPUT)
        return -EINVAL;

    buf = memdup_user(input, input_len);
    if (IS_ERR(buf))
        return PTR_ERR(buf);

    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!desc) {
        kfree(buf);
        return -ENOMEM;
    }

    desc->tfm = tfm;
    desc->flags = 0;
    ret = crypto_shash_digest(desc, buf, input_len, output);

    kfree(desc);
    kfree(buf);
    return ret;
}

static int tiny_hmac(struct crypto_shash *tfm, const void __user *key, u32 key_len,
                     const void __user *input, u32 input_len, void *output)
{
    struct shash_desc *desc;
    u8 *key_buf, *input_buf;
    int ret;

    if (!tfm || key_len > TINYCRYPTO_MAX_KEY_SIZE || input_len > TINYCRYPTO_MAX_INPUT)
        return -EINVAL;

    key_buf = memdup_user(key, key_len);
    if (IS_ERR(key_buf))
        return PTR_ERR(key_buf);

    input_buf = memdup_user(input, input_len);
    if (IS_ERR(input_buf)) {
        kfree(key_buf);
        return PTR_ERR(input_buf);
    }

    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!desc) {
        kfree(input_buf);
        kfree(key_buf);
        return -ENOMEM;
    }

    desc->tfm = tfm;
    desc->flags = 0;

    ret = crypto_shash_setkey(tfm, key_buf, key_len);
    if (!ret)
        ret = crypto_shash_digest(desc, input_buf, input_len, output);

    kfree(desc);
    kfree(input_buf);
    kfree(key_buf);
    return ret;
}

static int tiny_gcm(struct tinycrypto_gcm_req *req, bool decrypt)
{
    struct aead_request *areq = NULL;
    struct scatterlist sg;
    u8 *buf = NULL, *aad = NULL;
    u32 crypt_len;
    u32 total_len;
    int ret;

    if (req->key_len != 16 && req->key_len != 24 && req->key_len != 32)
        return -EINVAL;
    if (req->data_len > TINYCRYPTO_MAX_INPUT || req->aad_len > TINYCRYPTO_MAX_AAD)
        return -EINVAL;

    crypt_len = req->data_len + (decrypt ? TINYCRYPTO_GCM_TAG_SIZE : 0);
    total_len = req->aad_len + crypt_len;

    buf = kzalloc(total_len, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    if (req->aad_len) {
        aad = memdup_user(u64_to_user_ptr(req->aad), req->aad_len);
        if (IS_ERR(aad)) {
            ret = PTR_ERR(aad);
            aad = NULL;
            goto out;
        }
        memcpy(buf, aad, req->aad_len);
    }

    if (copy_from_user(buf + req->aad_len, u64_to_user_ptr(req->data), req->data_len)) {
        ret = -EFAULT;
        goto out;
    }

    if (decrypt)
        memcpy(buf + req->aad_len + req->data_len, req->tag, TINYCRYPTO_GCM_TAG_SIZE);

    ret = crypto_aead_setkey(gcm_tfm, req->key, req->key_len);
    if (ret)
        goto out;

    ret = crypto_aead_setauthsize(gcm_tfm, TINYCRYPTO_GCM_TAG_SIZE);
    if (ret)
        goto out;

    areq = aead_request_alloc(gcm_tfm, GFP_KERNEL);
    if (!areq) {
        ret = -ENOMEM;
        goto out;
    }

    sg_init_one(&sg, buf, total_len);
    aead_request_set_callback(areq, CRYPTO_TFM_REQ_MAY_SLEEP, NULL, NULL);
    aead_request_set_crypt(areq, &sg, &sg, crypt_len, req->iv);
    aead_request_set_ad(areq, req->aad_len);

    ret = decrypt ? crypto_aead_decrypt(areq) : crypto_aead_encrypt(areq);
    if (ret)
        goto out;

    if (copy_to_user(u64_to_user_ptr(req->data), buf + req->aad_len,
                     decrypt ? req->data_len : req->data_len)) {
        ret = -EFAULT;
        goto out;
    }

    if (!decrypt)
        memcpy(req->tag, buf + req->aad_len + req->data_len, TINYCRYPTO_GCM_TAG_SIZE);

out:
    aead_request_free(areq);
    kfree(aad);
    kfree(buf);
    return ret;
}

static long tinycrypto_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    mutex_lock(&crypto_lock);

    switch (cmd) {
    case TINYCRYPTO_SHA1: {
        struct tinycrypto_hash_req req = {};
        if (copy_from_user(&req, (void __user *) arg, sizeof(req))) { ret = -EFAULT; break; }
        ret = tiny_hash(sha1_tfm, u64_to_user_ptr(req.input), req.input_len, req.output);
        if (!ret && copy_to_user((void __user *) arg, &req, sizeof(req))) ret = -EFAULT;
        break;
    }
    case TINYCRYPTO_SHA256: {
        struct tinycrypto_hash_req req = {};
        if (copy_from_user(&req, (void __user *) arg, sizeof(req))) { ret = -EFAULT; break; }
        ret = tiny_hash(sha256_tfm, u64_to_user_ptr(req.input), req.input_len, req.output);
        if (!ret && copy_to_user((void __user *) arg, &req, sizeof(req))) ret = -EFAULT;
        break;
    }
    case TINYCRYPTO_HMAC_SHA1: {
        struct tinycrypto_hmac_req req = {};
        if (copy_from_user(&req, (void __user *) arg, sizeof(req))) { ret = -EFAULT; break; }
        ret = tiny_hmac(hmac_sha1_tfm, u64_to_user_ptr(req.key), req.key_len,
                        u64_to_user_ptr(req.input), req.input_len, req.output);
        if (!ret && copy_to_user((void __user *) arg, &req, sizeof(req))) ret = -EFAULT;
        break;
    }
    case TINYCRYPTO_HMAC_SHA256: {
        struct tinycrypto_hmac_req req = {};
        if (copy_from_user(&req, (void __user *) arg, sizeof(req))) { ret = -EFAULT; break; }
        ret = tiny_hmac(hmac_sha256_tfm, u64_to_user_ptr(req.key), req.key_len,
                        u64_to_user_ptr(req.input), req.input_len, req.output);
        if (!ret && copy_to_user((void __user *) arg, &req, sizeof(req))) ret = -EFAULT;
        break;
    }
    case TINYCRYPTO_AES_GCM_ENC:
    case TINYCRYPTO_AES_GCM_DEC: {
        struct tinycrypto_gcm_req req = {};
        if (copy_from_user(&req, (void __user *) arg, sizeof(req))) { ret = -EFAULT; break; }
        ret = tiny_gcm(&req, cmd == TINYCRYPTO_AES_GCM_DEC);
        if (!ret && copy_to_user((void __user *) arg, &req, sizeof(req))) ret = -EFAULT;
        break;
    }
    case TINYCRYPTO_RANDOM: {
        struct tinycrypto_random_req req = {};
        u8 *buf;
        if (copy_from_user(&req, (void __user *) arg, sizeof(req))) { ret = -EFAULT; break; }
        if (!req.output_len || req.output_len > TINYCRYPTO_MAX_INPUT) { ret = -EINVAL; break; }
        buf = kmalloc(req.output_len, GFP_KERNEL);
        if (!buf) { ret = -ENOMEM; break; }
        get_random_bytes(buf, req.output_len);
        if (copy_to_user(u64_to_user_ptr(req.output), buf, req.output_len)) ret = -EFAULT;
        kfree(buf);
        break;
    }
    default:
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&crypto_lock);
    return ret;
}

static const struct file_operations tinycrypto_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = tinycrypto_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = tinycrypto_ioctl,
#endif
};

static struct miscdevice tinycrypto_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "tinycrypto",
    .fops = &tinycrypto_fops,
    .mode = 0600,
};

static int __init tinycrypto_init(void)
{
    int ret;

    sha1_tfm = crypto_alloc_shash("sha1", 0, 0);
    sha256_tfm = crypto_alloc_shash("sha256", 0, 0);
    hmac_sha1_tfm = crypto_alloc_shash("hmac(sha1)", 0, 0);
    hmac_sha256_tfm = crypto_alloc_shash("hmac(sha256)", 0, 0);
    gcm_tfm = crypto_alloc_aead("gcm(aes)", 0, 0);

    if (IS_ERR(sha1_tfm) || IS_ERR(sha256_tfm) || IS_ERR(hmac_sha1_tfm) || IS_ERR(hmac_sha256_tfm) || IS_ERR(gcm_tfm)) {
        ret = -ENODEV;
        goto fail;
    }

    ret = misc_register(&tinycrypto_dev);
    if (ret)
        goto fail;

    return 0;

fail:
    if (!IS_ERR_OR_NULL(gcm_tfm)) crypto_free_aead(gcm_tfm);
    if (!IS_ERR_OR_NULL(hmac_sha256_tfm)) crypto_free_shash(hmac_sha256_tfm);
    if (!IS_ERR_OR_NULL(hmac_sha1_tfm)) crypto_free_shash(hmac_sha1_tfm);
    if (!IS_ERR_OR_NULL(sha256_tfm)) crypto_free_shash(sha256_tfm);
    if (!IS_ERR_OR_NULL(sha1_tfm)) crypto_free_shash(sha1_tfm);
    return ret;
}

static void __exit tinycrypto_exit(void)
{
    misc_deregister(&tinycrypto_dev);
    crypto_free_aead(gcm_tfm);
    crypto_free_shash(hmac_sha256_tfm);
    crypto_free_shash(hmac_sha1_tfm);
    crypto_free_shash(sha256_tfm);
    crypto_free_shash(sha1_tfm);
}

module_init(tinycrypto_init);
module_exit(tinycrypto_exit);
