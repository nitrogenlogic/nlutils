/*
 * Tests URL encoding/decoding functions.
 * Copyright (C)2014 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nlutils.h"

struct url_test {
	const char *desc;
	const char *input;
	const char *enc[2][2]; //row is space, column is reserved
	const char *dec;
};

const struct url_test urltests[] = {
	{
		.desc = "Empty string",
		.input = "",
		.enc = {
			{ "", "" }, // s0r0, s0r1
			{ "", "" } // s1r0, s1r1
		},
		.dec = "",
	},
	{
		.desc = "Simple decode",
		.input = "%20%40%60",
		.enc = {
			{ "%2520%2540%2560", "%2520%2540%2560" },
			{ "%2520%2540%2560", "%2520%2540%2560" }
		},
		.dec = " @`",
	},
	{
		.desc = "Simple encode",
		.input = "\t/ \n?",
		.enc = {
			{ "%09%2f+%0a%3f", "%09/+%0a?" },
			{ "%09%2f%20%0a%3f", "%09/%20%0a?" }
		},
		.dec = "\t/ \n?",
	},
	{
		.desc = "Practical simple URL",
		.input = "http://www.nitrogenlogic.com/",
		.enc = {
			{ "http%3a%2f%2fwww.nitrogenlogic.com%2f", "http://www.nitrogenlogic.com/" },
			{ "http%3a%2f%2fwww.nitrogenlogic.com%2f", "http://www.nitrogenlogic.com/" }
		},
		.dec = "http://www.nitrogenlogic.com/",
	},
	{
		.desc = "Practical complex URL",
		.input = "http://a:b@www.nitrogenlogic.com/?ign_parameter=1&also_ign=2;ign_final[]=3#footer",
		.enc = {
			{
				"http%3a%2f%2fa%3ab%40www.nitrogenlogic.com%2f%3fign_parameter%3d1%26also_ign%3d2%3bign_final%5b%5d%3d3%23footer",
				"http://a:b@www.nitrogenlogic.com/?ign_parameter=1&also_ign=2;ign_final[]=3#footer"
			},
			{
				"http%3a%2f%2fa%3ab%40www.nitrogenlogic.com%2f%3fign_parameter%3d1%26also_ign%3d2%3bign_final%5b%5d%3d3%23footer",
				"http://a:b@www.nitrogenlogic.com/?ign_parameter=1&also_ign=2;ign_final[]=3#footer"
			}
		},
		.dec = "http://a:b@www.nitrogenlogic.com/?ign_parameter=1&also_ign=2;ign_final[]=3#footer",
	},
	{
		.desc = "All nonzero characters",
		.input = (const char[]){
			// Generated with Ruby:
			// 255.times do |x|
			//   STDOUT.write "#{'%3d' % (x + 1)},#{x % 10 == 9 ? "\n" : ' '}"
			// end
			  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,
			 11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
			 21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
			 31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
			 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,
			 51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
			 61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
			 71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
			 81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
			 91,  92,  93,  94,  95,  96,  97,  98,  99, 100,
			101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
			111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
			121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
			131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
			141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
			151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
			161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
			171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
			181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
			191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
			201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
			211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
			221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
			231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
			241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
			251, 252, 253, 254, 255, 0
		},
		.enc = {
			{
				// Space=0 reserved=0 (Ruby CGI.escape except ~)
				"%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f"
				"%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%1f"
				"+%21%22%23%24%25%26%27%28%29%2a%2b%2c-.%2f"
				"0123456789%3a%3b%3c%3d%3e%3f"
				"%40ABCDEFGHIJKLMNO"
				"PQRSTUVWXYZ%5b%5c%5d%5e_"
				"%60abcdefghijklmno"
				"pqrstuvwxyz%7b%7c%7d~%7f"
				"%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f"
				"%90%91%92%93%94%95%96%97%98%99%9a%9b%9c%9d%9e%9f"
				"%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af"
				"%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%ba%bb%bc%bd%be%bf"
				"%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf"
				"%d0%d1%d2%d3%d4%d5%d6%d7%d8%d9%da%db%dc%dd%de%df"
				"%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef"
				"%f0%f1%f2%f3%f4%f5%f6%f7%f8%f9%fa%fb%fc%fd%fe%ff",

				// Space=0 reserved=1
				"%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f"
				"%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%1f"
				"+!%22#$%25&'()*+,-./"
				"0123456789:;%3c=%3e?"
				"@ABCDEFGHIJKLMNO"
				"PQRSTUVWXYZ[%5c]%5e_"
				"%60abcdefghijklmno"
				"pqrstuvwxyz%7b%7c%7d~%7f"
				"%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f"
				"%90%91%92%93%94%95%96%97%98%99%9a%9b%9c%9d%9e%9f"
				"%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af"
				"%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%ba%bb%bc%bd%be%bf"
				"%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf"
				"%d0%d1%d2%d3%d4%d5%d6%d7%d8%d9%da%db%dc%dd%de%df"
				"%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef"
				"%f0%f1%f2%f3%f4%f5%f6%f7%f8%f9%fa%fb%fc%fd%fe%ff",
			},
			{
				// Space=1 reserved=0
				"%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f"
				"%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%1f"
				"%20%21%22%23%24%25%26%27%28%29%2a%2b%2c-.%2f"
				"0123456789%3a%3b%3c%3d%3e%3f"
				"%40ABCDEFGHIJKLMNO"
				"PQRSTUVWXYZ%5b%5c%5d%5e_"
				"%60abcdefghijklmno"
				"pqrstuvwxyz%7b%7c%7d~%7f"
				"%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f"
				"%90%91%92%93%94%95%96%97%98%99%9a%9b%9c%9d%9e%9f"
				"%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af"
				"%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%ba%bb%bc%bd%be%bf"
				"%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf"
				"%d0%d1%d2%d3%d4%d5%d6%d7%d8%d9%da%db%dc%dd%de%df"
				"%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef"
				"%f0%f1%f2%f3%f4%f5%f6%f7%f8%f9%fa%fb%fc%fd%fe%ff",

				// Space=1 reserved=1 (Ruby URI.encode except #)
				"%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f"
				"%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%1f"
				"%20!%22#$%25&'()*+,-./"
				"0123456789:;%3c=%3e?"
				"@ABCDEFGHIJKLMNO"
				"PQRSTUVWXYZ[%5c]%5e_"
				"%60abcdefghijklmno"
				"pqrstuvwxyz%7b%7c%7d~%7f"
				"%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f"
				"%90%91%92%93%94%95%96%97%98%99%9a%9b%9c%9d%9e%9f"
				"%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af"
				"%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%ba%bb%bc%bd%be%bf"
				"%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf"
				"%d0%d1%d2%d3%d4%d5%d6%d7%d8%d9%da%db%dc%dd%de%df"
				"%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef"
				"%f0%f1%f2%f3%f4%f5%f6%f7%f8%f9%fa%fb%fc%fd%fe%ff",
			}
		},
		.dec = (const char[]){
			  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,
			 11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
			 21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
			 31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
			 41,  42,  32,  44,  45,  46,  47,  48,  49,  50, // + was 43, becomes ' ' (32)
			 51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
			 61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
			 71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
			 81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
			 91,  92,  93,  94,  95,  96,  97,  98,  99, 100,
			101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
			111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
			121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
			131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
			141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
			151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
			161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
			171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
			181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
			191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
			201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
			211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
			221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
			231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
			241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
			251, 252, 253, 254, 255, 0
		},
	},
};

static int do_url_test(const struct url_test *test)
{
	char *enc, *dec;
	int space, reserved;

	for(space = 0; space <= 1; space++) {
		for(reserved = 0; reserved <= 1; reserved++) {
			enc = nl_url_encode(test->input, space, reserved);
			if(enc == NULL) {
				ERROR_OUT("NULL result for %s from nl_url_encode(\"%s\", %d, %d)\n",
						test->desc, test->input, space, reserved);
				return -1;
			}
			if(strcmp(enc, test->enc[space][reserved])) {
				ERROR_OUT("Mismatch for %s from nl_url_encode(\"%s\", %d, %d)\n",
						test->desc, test->input, space, reserved);
				ERROR_OUT("\tExpected %s, got %s\n", test->enc[space][reserved], enc);
				free(enc);
				return -1;
			}

			dec = nl_url_decode(enc, space);
			if(dec == NULL) {
				ERROR_OUT("NULL result for %s from nl_url_decode(, %d) after enc(%s, %d, %d)\n",
						test->desc, space, test->input, space, reserved);
				free(enc);
				free(dec);
				return -1;
			}
			if(reserved && !space && strchr(test->input, '+')) {
				INFO_OUT("Skipping decode-after-encode test for %s (space=%d, reserved=%d) because it contains a +.\n",
						test->desc, space, reserved);
			} else if(strcmp(dec, test->input)) {
				ERROR_OUT("Decode doesn't match original for %s (space=%d, reserved=%d)\n",
						test->desc, space, reserved);
				ERROR_OUT("\tExpected %s, got %s\n", test->input, dec);
				free(enc);
				free(dec);
				return -1;
			}

			free(enc);
			free(dec);
		}
	}

	dec = nl_url_decode(test->input, 0);
	if(dec == NULL) {
		ERROR_OUT("NULL result for %s from nl_url_decode(%s, 0)\n", test->desc, test->input);
		return -1;
	}
	if(strcmp(dec, test->dec)) {
		ERROR_OUT("Decoded value mismatch for %s (%s, 0)\n", test->desc, test->input);
		ERROR_OUT("\tExpected %s, got %s\n", test->dec, dec);
		free(dec);
		return -1;
	}

	free(dec);

	return 0;
}

int main(void)
{
	size_t i;
	char *str;

	INFO_OUT("Testing NULL parameters to encode and decode\n");
	if((str = nl_url_encode(NULL, 0, 0)) != NULL) {
		ERROR_OUT("Expected NULL from nl_url_encode(), got %s\n", str);
		free(str);
		return -1;
	}
	if((str = nl_url_encode(NULL, 0, 0)) != NULL) {
		ERROR_OUT("Expected NULL from nl_url_encode(), got %s\n", str);
		free(str);
		return -1;
	}

	for(i = 0; i < ARRAY_SIZE(urltests); i++) {
		if(do_url_test(&urltests[i])) {
			ERROR_OUT("URL encode/decode tests failed.\n");
			return -1;
		}
	}

	INFO_OUT("URL encode/decode tests passed.\n");

	return 0;
}
