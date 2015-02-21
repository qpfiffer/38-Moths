// vim: noet ts=4 sw=4
#pragma once

/* Size of BMW hash */
#define IMAGE_HASH_SIZE 256
#define HASH_ARRAY_SIZE IMAGE_HASH_SIZE/8
#define HASH_IMAGE_STR_SIZE (HASH_ARRAY_SIZE * 2) + 1

#define MAX_KEY_SIZE 250
/* Namespaces for when we create our keys */
#define WAIFU_NMSPC "waifu"
#define WEBM_NMSPC "webm"
#define ALIAS_NMSPC "alias"
#define WEBMTOALIAS_NMSPC "W2A"

#define MAX_IMAGE_FILENAME_SIZE 255
#define MAX_BOARD_NAME_SIZE 16
