/*
 * Generic network-related functions, excluding actual network communication.
 * Copyright (C)2011 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_NET_H_
#define NLUTILS_NET_H_

/*
 * Parses the MAC address in mac_in into an unbroken string of lowercase
 * hexadecimal digits in the buffer pointed to by mac_out.  If mac_out is NULL,
 * the formatting of the MAC address is verified, but the parsed MAC address is
 * not stored anywhere.  If separator is not zero, then it will be inserted
 * between each octet in the parsed MAC address.  The buffer pointed to by
 * mac_out must be able to hold at least 13 bytes, or 18 bytes if separator is
 * not zero.  mac_out will not be modified if the MAC address is invalid.
 * Returns 0 on success, -EINVAL on invalid MAC format, -EFAULT if mac_in is
 * NULL (a simple check for zero is sufficient to detect errors).
 */
int nl_parse_mac(const char *mac_in, char *mac_out, char separator);

#endif /* NLUTILS_NET_H_ */
