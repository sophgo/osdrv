// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <stdint.h>
#include <string.h>

#define u32 uint32_t
#define u64 uint64_t
#include "sophon_spacc.h"

#define POOL_SIZE (100 * 1024)
#define DATA_SIZE (60 * 1024)

int testBase64(int argc, char **args)
{
	int fd = -1;
	unsigned int result_size;
	int ret;
	unsigned int pool_size = POOL_SIZE;
	char buf[POOL_SIZE] = { 0 };
	struct cvi_spacc_base64 b64 = { 0, 1 };

	fd = open("/dev/spacc", O_RDWR);
	if (fd < 0) {
		printf("open /dev/spacc failed\n");
		return -1;
	}

	if (ioctl(fd, IOCTL_SPACC_CREATE_MEMPOOL, &pool_size)) {
		printf("ioctl failed\n");
		return -1;
	}

	pool_size = 0;
	ioctl(fd, IOCTL_SPACC_GET_MEMPOOL_SIZE, &pool_size);
	printf("pool size: %d\n", pool_size);

	memcpy(buf, "hello", 5);

	// Encode
	write(fd, buf, DATA_SIZE);

	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	// Decode
	write(fd, buf, result_size);
	b64.action = 0;

	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);
	buf[result_size] = 0;

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	// Customer code Encode
	buf[0] = 0xFB;
	buf[1] = 0xEF;
	buf[2] = 0xBE;

	write(fd, buf, 3);

	b64.customer_code = ('!' << 8) | '#';
	b64.action = 1;
	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);
	buf[result_size] = 0;

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	// Customer code Decode
	memcpy(buf, "####", 4);

	write(fd, buf, 4);

	b64.customer_code = ('!' << 8) | '#';
	b64.action = 0;
	result_size = ioctl(fd, IOCTL_SPACC_BASE64, &b64);
	printf("result_size : %d\n", result_size);
	buf[result_size] = 0;

	ret = read(fd, buf, result_size);
	printf("ret : %d, %s\n", ret, buf);

	int i = 0;

	for (; i < 3; i++)
		printf("0x%x ", buf[i]);

	printf("\n");
	close(fd);
	return ret;
}
int testAES_CTR(void)
{
    int fd = -1;
    int ret = -1;
    FILE *fp_in = NULL, *fp_out = NULL, *fp_dec = NULL;
    struct spacc_aes_config conf;
    // const size_t BLOCK_SIZE = 4 * 1024 * 1024;  // 4MB block size
    const size_t BLOCK_SIZE = 32 * 1024;  // 32KB block size

    const size_t TOTAL_SIZE = 100 * 1024 * 1024;  // 100MB total size
    unsigned int pool_size = BLOCK_SIZE;
    unsigned char *buf = malloc(BLOCK_SIZE);
    size_t remaining = TOTAL_SIZE;
    size_t processed = 0;
    uint64_t block_counter = 0;  // Used to track block count

    if (!buf) {
        printf("Failed to allocate memory\n");
        return -1;
    }

    // Open input file
    fp_in = fopen("100MB.bin", "rb");
    if (!fp_in) {
        printf("Failed to open 100MB.bin\n");
        goto cleanup;
    }

    // Create encrypted output file
    fp_out = fopen("spacc_enc_ctr.bin", "wb");
    if (!fp_out) {
        printf("Failed to create encrypted output file\n");
        goto cleanup;
    }

    // Open device
    fd = open("/dev/spacc", O_RDWR);
    if (fd < 0) {
        printf("open /dev/spacc failed\n");
        goto cleanup;
    }

    // Create memory pool
    if (ioctl(fd, IOCTL_SPACC_CREATE_MEMPOOL, &pool_size)) {
        printf("Failed to create memory pool\n");
        goto cleanup;
    }

    unsigned char key[32] = {
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61
    };

    // Base counter/IV for CTR mode
    // unsigned char base_counter[16] = {
    //     0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    //     0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    // };
    unsigned char base_counter[16] = {
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61
    };
    // Current block counter
    unsigned char current_counter[16];

    uint64_t aes_blocks_per_chunk = BLOCK_SIZE / 16;

    printf("Each block contains %llu AES blocks (16 bytes each)\n", aes_blocks_per_chunk);

    printf("encrypt case (CTR mode):============================\n");

    // Process each 32KB block
    while (remaining > 0) {
        // Calculate current block size
        size_t current_block = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;

        // Read a block of data
        size_t read_size = fread(buf, 1, current_block, fp_in);
        if (read_size != current_block) {
            printf("Read error: expected %zu, got %zu\n", current_block, read_size);
            goto cleanup;
        }

        // Number of AES blocks processed
        uint64_t aes_blocks_processed = block_counter * aes_blocks_per_chunk;

        // Check for counter overflow
        uint64_t max_counter = UINT64_MAX - aes_blocks_per_chunk;
        if (aes_blocks_processed > max_counter) {
            printf("Counter overflow detected! Need to reset encryption state.\n");
            goto cleanup;
        }

        // Calculate counter value for current block
        memcpy(current_counter, base_counter, 16);

        // Add processed blocks count to counter with carry handling
        uint64_t counter_value = aes_blocks_processed;
        uint8_t carry = 0;
        for (int i = 15; i >= 8; i--) {
            uint16_t sum = current_counter[i] + (counter_value & 0xFF) + carry;
            current_counter[i] = sum & 0xFF;
            carry = sum >> 8;
            counter_value >>= 8;
        }

        // If there's still a carry, handle the first 8 bytes
        if (carry) {
            for (int i = 7; i >= 0 && carry; i--) {
                uint16_t sum = current_counter[i] + carry;
                current_counter[i] = sum & 0xFF;
                carry = sum >> 8;
            }
        }

        // Write data to device
        size_t written = write(fd, buf, read_size);
        if (written != read_size) {
            printf("Write to device failed\n");
            goto cleanup;
        }

        // Configure encryption parameters - using CTR mode
        memset(&conf, 0, sizeof(spacc_aes_config_s));
        conf.mode = SPACC_ALGO_MODE_CTR;  // Use CTR mode
        conf.key_mode = SPACC_KEY_SIZE_256BITS;
        conf.action = SPACC_ACTION_ENCRYPTION;  // In CTR mode, encryption and decryption operations are the same
        conf.otp = SPACC_KEY_SOURCE_DESCRIPTOR;
        conf.key = (uintptr_t)key;
        conf.iv = (uintptr_t)current_counter;  // Use current block's counter

        // Execute encryption
        if (ioctl(fd, IOCTL_SPACC_AES_ACTION, &conf) < 0) {
            printf("SPACC_AES_ACTION failed\n");
            goto cleanup;
        }

        // Read encrypted data
        ret = read(fd, buf, read_size);
        if (ret < 0) {
            printf("Read from device failed\n");
            goto cleanup;
        }

        // Write encrypted data to file
        size_t file_written = fwrite(buf, 1, ret, fp_out);
        if (file_written != ret) {
            printf("Write to file failed\n");
            goto cleanup;
        }

        // Update block counter
        block_counter++;

        remaining -= read_size;
        processed += read_size;
        printf("Encrypted %zu bytes (%.1f%%)\n", processed,
            (float)processed * 100 / TOTAL_SIZE);

        // Display counter value for each block
        printf("Block %llu counter: ", block_counter);
        for (int i = 0; i < 16; i++) {
            printf("%02x ", current_counter[i]);
        }
        printf("\n");
    }

    // Close files, prepare for decryption
    fclose(fp_in);
    fp_in = NULL;
    fclose(fp_out);
    fp_out = NULL;

    printf("decrypt case (CTR mode):============================\n");

    // Reset counters
    remaining = TOTAL_SIZE;
    processed = 0;
    block_counter = 0;  // Reset block counter

    // Reopen encrypted file
    fp_in = fopen("spacc_enc_ctr.bin", "rb");
    if (!fp_in) {
        printf("Failed to open encrypted file\n");
        goto cleanup;
    }

    // Create decrypted output file
    fp_out = fopen("spacc_dec_ctr.bin", "wb");
    if (!fp_out) {
        printf("Failed to create decrypted output file\n");
        goto cleanup;
    }

    // Decryption loop - in CTR mode, encryption and decryption operations are the same
    while (remaining > 0) {
        size_t current_block = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;

        // Read encrypted data
        size_t read_size = fread(buf, 1, current_block, fp_in);
        if (read_size != current_block) {
            printf("Read error during decrypt: expected %zu, got %zu\n",
                    current_block, read_size);
            goto cleanup;
        }

        // Calculate counter value for current block - same as during encryption
        memcpy(current_counter, base_counter, 16);

        // Number of AES blocks processed - using same logic as encryption
        uint64_t aes_blocks_processed = block_counter * aes_blocks_per_chunk;

        // Add processed blocks count to counter with carry handling
        uint64_t counter_value = aes_blocks_processed;
        uint8_t carry = 0;
        for (int i = 15; i >= 8; i--) {
            uint16_t sum = current_counter[i] + (counter_value & 0xFF) + carry;
            current_counter[i] = sum & 0xFF;
            carry = sum >> 8;
            counter_value >>= 8;
        }

        // If there's still a carry, handle the first 8 bytes
        if (carry) {
            for (int i = 7; i >= 0 && carry; i--) {
                uint16_t sum = current_counter[i] + carry;
                current_counter[i] = sum & 0xFF;
                carry = sum >> 8;
            }
        }

        // Write data to device
        size_t written = write(fd, buf, read_size);
        if (written != read_size) {
            printf("Write to device failed during decrypt\n");
            goto cleanup;
        }

        // Configure decryption parameters - in CTR mode, encryption and decryption are the same
        memset(&conf, 0, sizeof(spacc_aes_config_s));
        conf.mode = SPACC_ALGO_MODE_CTR;
        conf.key_mode = SPACC_KEY_SIZE_256BITS;
        conf.action = SPACC_ACTION_DECRYPT;  // In CTR mode, encryption and decryption operations are the same
        conf.otp = SPACC_KEY_SOURCE_DESCRIPTOR;
        conf.key = (uintptr_t)key;
        conf.iv = (uintptr_t)current_counter;

        // Execute decryption
        if (ioctl(fd, IOCTL_SPACC_AES_ACTION, &conf) < 0) {
            printf("SPACC_AES_ACTION decrypt failed\n");
            goto cleanup;
        }

        // Read decrypted data
        ret = read(fd, buf, read_size);
        if (ret < 0) {
            printf("Read from device failed during decrypt\n");
            goto cleanup;
        }

        // Write decrypted data to file
        size_t file_written = fwrite(buf, 1, ret, fp_out);
        if (file_written != ret) {
            printf("Write to decrypted file failed\n");
            goto cleanup;
        }

        // Update block counter
        block_counter++;

        remaining -= read_size;
        processed += read_size;
        printf("Decrypted %zu bytes (%.1f%%)\n", processed,
                (float)processed * 100 / TOTAL_SIZE);

        // Display counter value for each block
        printf("Block %llu counter: ", block_counter);
        for (int i = 0; i < 16; i++) {
            printf("%02x ", current_counter[i]);
        }
        printf("\n");
    }

    printf("Decryption completed successfully\n");

    // Verify decryption result
    printf("Verifying decryption result...\n");

    // Reopen original and decrypted files for comparison
    fclose(fp_in);
    fp_in = NULL;
    fclose(fp_out);
    fp_out = NULL;

    fp_in = fopen("100MB.bin", "rb");
    fp_dec = fopen("spacc_dec_ctr.bin", "rb");
    if (!fp_in || !fp_dec) {
        printf("Failed to open files for verification\n");
        goto cleanup;
    }

    remaining = TOTAL_SIZE;
    processed = 0;

    // Verify file contents in chunks
    while (remaining > 0) {
        unsigned char orig_buf[4096];
        unsigned char dec_buf[4096];

        size_t verify_size = (remaining > sizeof(orig_buf)) ?
                    sizeof(orig_buf) : remaining;

        size_t orig_read = fread(orig_buf, 1, verify_size, fp_in);
        size_t dec_read = fread(dec_buf, 1, verify_size, fp_dec);

        if (orig_read != dec_read || orig_read != verify_size) {
            printf("Verification failed: size mismatch at offset %zu\n", processed);
            goto cleanup;
        }

        if (memcmp(orig_buf, dec_buf, verify_size) != 0) {
            printf("Verification failed: content mismatch at offset %zu\n", processed);
            goto cleanup;
        }

        remaining -= verify_size;
        processed += verify_size;

        if (processed % (1024*1024) == 0) {  // Show progress every 1MB
            printf("Verified %zu bytes (%.1f%%)\n", processed,
                    (float)processed * 100 / TOTAL_SIZE);
        }
    }

    printf("Verification successful: %zu bytes match\n", TOTAL_SIZE);
    ret = 0;

cleanup:
    if (fp_in)
        fclose(fp_in);
    if (fp_out)
        fclose(fp_out);
    if (fp_dec)
        fclose(fp_dec);
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    return ret;
}
int testAES_CBC(void)
{
    int fd = -1;
    int ret = -1;  // Set default return value to failure
    FILE *fp_in = NULL, *fp_out = NULL, *fp_dec = NULL;
    spacc_aes_config_s conf;
    const size_t BLOCK_SIZE = 4 * 1024 * 1024;  // 4MB block size
    const size_t TOTAL_SIZE = 100 * 1024 * 1024;  // 100MB total size
    unsigned int pool_size = BLOCK_SIZE;
    unsigned char *buf = malloc(BLOCK_SIZE);
    size_t remaining = TOTAL_SIZE;
    size_t processed = 0;

    if (!buf) {
        printf("Failed to allocate memory\n");
        return -1;
    }

    // Open input file
    fp_in = fopen("100MB.bin", "rb");
    if (!fp_in) {
        printf("Failed to open 100MB.bin\n");
        goto cleanup;
    }

    // Create encrypted output file
    fp_out = fopen("spacc_enc.bin", "wb");
    if (!fp_out) {
        printf("Failed to create encrypted output file\n");
        goto cleanup;
    }

    // Open device
    fd = open("/dev/spacc", O_RDWR);
    if (fd < 0) {
        printf("open /dev/spacc failed\n");
        goto cleanup;
    }

    // Create memory pool
    if (ioctl(fd, IOCTL_SPACC_CREATE_MEMPOOL, &pool_size)) {
        printf("Failed to create memory pool\n");
        goto cleanup;
    }

    unsigned char key[32] = {
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61
    };
    unsigned char iv[16] = {
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
        0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61
    };

    // Process each 4MB block
    while (remaining > 0) {
        // Calculate current block size
        size_t current_block = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;

        // Read a block of data
        size_t read_size = fread(buf, 1, current_block, fp_in);
        if (read_size != current_block) {
            printf("Read error: expected %zu, got %zu\n", current_block, read_size);
            goto cleanup;
        }

        // Write data to device
        size_t written = write(fd, buf, read_size);
        if (written != read_size) {
            printf("Write to device failed\n");
            goto cleanup;
        }

        // Configure encryption parameters
        memset(&conf, 0, sizeof(spacc_aes_config_s));
        conf.mode = SPACC_ALGO_MODE_CBC;
        conf.key_mode = SPACC_KEY_SIZE_256BITS;
        conf.action = SPACC_ACTION_ENCRYPTION;
        conf.otp = SPACC_KEY_SOURCE_DESCRIPTOR;
        conf.key = (uintptr_t)key;
        conf.iv = (uintptr_t)iv;

        // Execute encryption
        if (ioctl(fd, IOCTL_SPACC_AES_ACTION, &conf) < 0) {
            printf("SPACC_AES_ACTION failed\n");
            goto cleanup;
        }

        // Read encrypted data
        ret = read(fd, buf, read_size);
        if (ret < 0) {
            printf("Read from device failed\n");
            goto cleanup;
        }

        // Save last 16 bytes as IV for next block
        if (remaining > BLOCK_SIZE) {
            memcpy(iv, buf + ret - 16, 16);
        }

        // Write encrypted data to file
        size_t file_written = fwrite(buf, 1, ret, fp_out);
        if (file_written != ret) {
            printf("Write to file failed\n");
            goto cleanup;
        }

        remaining -= read_size;
        processed += read_size;
        printf("Processed %zu bytes (%.1f%%)\n", processed,
            (float)processed * 100 / TOTAL_SIZE);
    }

    // Close files, prepare for decryption
    fclose(fp_in);
    fp_in = NULL;
    fclose(fp_out);
    fp_out = NULL;
    printf("decrypt case:============================\n");

    // Reset counters
    remaining = TOTAL_SIZE;
    processed = 0;

    // Reset IV to initial value
    memcpy(iv, (unsigned char[16]){
            0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
            0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61
            },
            16);

    // Reopen encrypted file
    fp_in = fopen("spacc_enc.bin", "rb");
    if (!fp_in) {
        printf("Failed to open encrypted file\n");
        goto cleanup;
    }

    // Create decrypted output file
    fp_out = fopen("spacc_dec.bin", "wb");
    if (!fp_out) {
        printf("Failed to create decrypted output file\n");
        goto cleanup;
    }

    // Decryption loop
    while (remaining > 0) {
        size_t current_block = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;

        // Read encrypted data
        size_t read_size = fread(buf, 1, current_block, fp_in);
        if (read_size != current_block) {
            printf("Read error during decrypt: expected %zu, got %zu\n",
            current_block, read_size);
            goto cleanup;
        }

        // Save current block's ciphertext for next block's IV
        unsigned char next_iv[16];
        if (remaining > BLOCK_SIZE) {
            memcpy(next_iv, buf + read_size - 16, 16);
        }

        // Write data to device
        size_t written = write(fd, buf, read_size);
        if (written != read_size) {
            printf("Write to device failed during decrypt\n");
            goto cleanup;
        }

        // Configure decryption parameters
        memset(&conf, 0, sizeof(spacc_aes_config_s));
        conf.mode = SPACC_ALGO_MODE_CBC;
        conf.key_mode = SPACC_KEY_SIZE_256BITS;
        conf.action = SPACC_ACTION_DECRYPT;
        conf.otp = SPACC_KEY_SOURCE_DESCRIPTOR;
        conf.key = (uintptr_t)key;
        conf.iv = (uintptr_t)iv;

        // Execute decryption
        if (ioctl(fd, IOCTL_SPACC_AES_ACTION, &conf) < 0) {
            printf("SPACC_AES_ACTION decrypt failed\n");
            goto cleanup;
        }

        // Read decrypted data
        ret = read(fd, buf, read_size);
        if (ret < 0) {
            printf("Read from device failed during decrypt\n");
            goto cleanup;
        }

        // Write decrypted data to file
        size_t file_written = fwrite(buf, 1, ret, fp_out);
        if (file_written != ret) {
            printf("Write to decrypted file failed\n");
            goto cleanup;
        }

        // Update IV with current block's ciphertext
        if (remaining > BLOCK_SIZE) {
            memcpy(iv, next_iv, 16);
        }

        remaining -= read_size;
        processed += read_size;
        printf("Decrypted %zu bytes (%.1f%%)\n", processed,
                (float)processed * 100 / TOTAL_SIZE);
    }

    printf("Decryption completed successfully\n");

    // Verify decryption result
    printf("Verifying decryption result...\n");

    // Reopen original and decrypted files for comparison
    fclose(fp_in);
    fp_in = NULL;
    fclose(fp_out);
    fp_out = NULL;

    // Verify decrypted result
    fp_in = fopen("100MB.bin", "rb");
    fp_dec = fopen("spacc_dec.bin", "rb");
    if (!fp_in || !fp_dec) {
        printf("Failed to open files for verification\n");
        goto cleanup;
    }

    remaining = TOTAL_SIZE;
    processed = 0;

    // Verify file contents in chunks
    while (remaining > 0) {
        unsigned char orig_buf[4096];
        unsigned char dec_buf[4096];

        size_t verify_size = (remaining > sizeof(orig_buf)) ?
                    sizeof(orig_buf) : remaining;

        size_t orig_read = fread(orig_buf, 1, verify_size, fp_in);
        size_t dec_read = fread(dec_buf, 1, verify_size, fp_dec);

        if (orig_read != dec_read || orig_read != verify_size) {
            printf("Verification failed: size mismatch at offset %zu\n", processed);
            goto cleanup;
        }

        if (memcmp(orig_buf, dec_buf, verify_size) != 0) {
            printf("Verification failed: content mismatch at offset %zu\n", processed);
            goto cleanup;
        }

        remaining -= verify_size;
        processed += verify_size;

        if (processed % (1024*1024) == 0) {  // Show progress every 1MB
            printf("Verified %zu bytes (%.1f%%)\n", processed,
                    (float)processed * 100 / TOTAL_SIZE);
        }
    }

    printf("Verification successful: %zu bytes match\n", TOTAL_SIZE);
    ret = 0;

cleanup:
    if (fp_in)
        fclose(fp_in);
    if (fp_out)
        fclose(fp_out);
    if (fp_dec)
        fclose(fp_dec);
    if (fd >= 0)
        close(fd);
    if (buf)
        free(buf);
    return ret;
}
int main(int argc, char **args)
{
	testAES_CTR();
}
