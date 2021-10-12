/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* \summary: IPv6 routing header printer */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "netdissect-stdinc.h"

#include "netdissect.h"
#include "addrtoname.h"
#include "extract.h"

#include "ip6.h"

int
rt6_tlv_print(netdissect_options *ndo, const u_char *p, int bytes_left)
{
	int tlv_type, tlv_len;
	u_int8_t parse_next = 1;
	
	while (parse_next)
	{
		tlv_type = GET_S_1(p);
		ND_PRINT(", TLV-type=%u", tlv_type);
		p += 1;
		bytes_left -= 1;
		if (tlv_type == IPV6_RTHDR_TLV_TYPE_0)
			continue;
		if (bytes_left == 0) /* but it is not supposed to end with a padding TLV */
			break;
		tlv_len = GET_S_1(p);
		ND_PRINT(", TLV-len=%u", tlv_len);
		p += 1;
		if (tlv_len > bytes_left)
		{
			ND_PRINT(" (invalid TLV length %u)", tlv_len);
			return -1;
		}
		/////// to remove ///////
		parse_next = 0;
		////////////////////////
	}
	

	return 0;
}


int
rt6_print(netdissect_options *ndo, const u_char *bp, const u_char *bp2 _U_)
{
	const struct ip6_rthdr *dp;
	const struct ip6_rthdr0 *dp0;
	const struct ip6_srh *srh;
	u_int i, len, type, seg_list_len, last_entry;
	int err;
	const u_char *p;

	ndo->ndo_protocol = "rt6";

	nd_print_protocol_caps(ndo);
	dp = (const struct ip6_rthdr *)bp;

	len = GET_U_1(dp->ip6r_len);
	ND_PRINT(" (len=%u", len);	/*)*/
	type = GET_U_1(dp->ip6r_type);
	ND_PRINT(", type=%u", type);
	if (type == IPV6_RTHDR_TYPE_0)
		ND_PRINT(" [Deprecated]");
	ND_PRINT(", segleft=%u", GET_U_1(dp->ip6r_segleft));

	switch (type) {
	case IPV6_RTHDR_TYPE_0:
	case IPV6_RTHDR_TYPE_2:			/* Mobile IPv6 ID-20 */
		dp0 = (const struct ip6_rthdr0 *)dp;

		if (GET_BE_U_4(dp0->ip6r0_reserved) || ndo->ndo_vflag) {
			ND_PRINT(", rsv=0x%0x",
			    GET_BE_U_4(dp0->ip6r0_reserved));
		}

		if (len % 2 == 1) {
			ND_PRINT(" (invalid length %u)", len);
			goto invalid;
		}
		len >>= 1;
		p = (const u_char *) dp0->ip6r0_addr;
		for (i = 0; i < len; i++) {
			ND_PRINT(", [%u]%s", i, GET_IP6ADDR_STRING(p));
			p += 16;
		}
		/*(*/
		ND_PRINT(") ");
		return((GET_U_1(dp0->ip6r0_len) + 1) << 3);
		break;
	case IPV6_RTHDR_TYPE_4:
		srh = (const struct ip6_srh *)dp;
		last_entry = GET_U_1(srh->srh_last_ent);
		ND_PRINT(", last-entry=%u", last_entry);

		if (GET_U_1(srh->srh_flags) || ndo->ndo_vflag) {
			ND_PRINT(", flags=0x%0x",
				GET_U_1(srh->srh_flags));
		}

		ND_PRINT(", tag=%x", GET_BE_U_2(srh->srh_tag));
		p  = (const u_char *) srh->srh_segments;
		for (i = 0; i < last_entry + 1; i++)
		{
			ND_PRINT(", [%u]%s", i, GET_IP6ADDR_STRING(p));
			p += 16;
		}
		seg_list_len = (last_entry + 1) * 2;
		if (seg_list_len < len)
		{
			/* there is TLV */
			int bytes_left;
			bytes_left = (len - seg_list_len) * 8;
			err = rt6_tlv_print(ndo, p, bytes_left);
			if (err)
				goto invalid;
		}
		
		/*(*/
		ND_PRINT(") ");
		return((GET_U_1(srh->srh_len) + 1) << 3);
		break;
	default:
		ND_PRINT(" (unknown type)");
		goto invalid;
	}

invalid:
	nd_print_invalid(ndo);
	return -1;
}
