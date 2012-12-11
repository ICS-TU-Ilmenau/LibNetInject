/*****************************************************************************
 *
 *  Copyright (c) 2010 Thomas Volkert <thomas@homer-conferencing.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *****************************************************************************/

/*****************************************************************************
 * This file was copied from Homer-Conferencing (www.homer-conferencing.com).
 * It is hereby separately published under BSD license with the permission of
 * the original author.
 *****************************************************************************/

/*
 * Purpose: Extension for arpa/inet.h for MinGw environments
 * Author:  Thomas Volkert
 * Since:   2011-10-20
 */

#include <stdio.h>
#include <stdlib.h>
#include <endian.h>

namespace LibNetInject {

///////////////////////////////////////////////////////////////////////////////
//define some missing IP address service functions for MinGW environment
#ifdef __MINGW32__

#define NS_IN6ADDRSZ 		16	/*%< IPv6 T_AAAA */
#define NS_INT16SZ			2	/*%< #/bytes of data in a u_int16_t */

#undef htons
uint16_t htons(uint64_t pX)
{
    #if BYTE_ORDER == BIG_ENDIAN
        return pX;
    #elif BYTE_ORDER == LITTLE_ENDIAN
        return __bswap_16(pX);
    #else
        # error "What kind of system is this?"
    #endif
}

#undef ntohs
uint16_t ntohs(uint64_t pX)
{
    #if BYTE_ORDER == BIG_ENDIAN
        return pX;
    #elif BYTE_ORDER == LITTLE_ENDIAN
        return __bswap_16(pX);
    #else
        # error "What kind of system is this?"
    #endif
}

#undef inet_ntoa
static __thread char sBuffer[18];
char *inet_ntoa(struct in_addr pIn)
{
    unsigned char *tBytes = (unsigned char *) &pIn;
    snprintf(sBuffer, sizeof (sBuffer), "%d.%d.%d.%d", tBytes[0], tBytes[1], tBytes[2], tBytes[3]);

    return sBuffer;
}

#undef inet_ntop4
char *inet_ntop4(const u_char *pSrc, char *pDst, socklen_t pSize)
{
	char tmp[sizeof "255.255.255.255"];

	if (snprintf(tmp, "%u.%u.%u.%u", sizeof(tmp), pSrc[0], pSrc[1], pSrc[2], pSrc[3]) >= (int)pSize)
	{
		errno = ENOSPC;
		return NULL;
	}
	return strcpy(pDst, tmp);
}

#undef inet_ntop6
char *inet_ntop6(const u_char *pSrc, char *pDst, socklen_t pSize)
{
	char tTmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tTp;
	struct {
		int base, len;
	}tBest, tCurrent;
	u_int tWords[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	memset(tWords, '\0', sizeof tWords);
	for (i = 0; i < NS_IN6ADDRSZ; i += 2)
		tWords[i / 2] = (pSrc[i] << 8) | pSrc[i + 1];

	tBest.base = -1;
	tCurrent.base = -1;
	tBest.len = 0;
	tCurrent.len = 0;

	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
	{
		if (tWords[i] == 0)
		{
			if (tCurrent.base == -1)
				tCurrent.base = i, tCurrent.len = 1;
			else
				tCurrent.len++;
		} else
		{
			if (tCurrent.base != -1)
			{
				if (tBest.base == -1 || tCurrent.len > tBest.len)
					tBest = tCurrent;
				tCurrent.base = -1;
			}
		}
	}

	if (tCurrent.base != -1)
	{
		if (tBest.base == -1 || tCurrent.len > tBest.len)
			tBest = tCurrent;
	}
	if (tBest.base != -1 && tBest.len < 2)
		tBest.base = -1;

	tTp = tTmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
	{
		if (tBest.base != -1 && i >= tBest.base && i < (tBest.base + tBest.len))
		{
			if (i == tBest.base)
				*tTp++ = ':';
			continue;
		}
		if (i != 0)
			*tTp++ = ':';
		if (i == 6 && tBest.base == 0 && (tBest.len == 6 || (tBest.len == 5 && tWords[5] == 0xffff)))
		{
			if (!inet_ntop4(pSrc + 12, tTp, sizeof tTmp - (tTp - tTmp)))
				return (NULL);
			tTp += strlen(tTp);
			break;
		}
		tTp += snprintf(tTp, sizeof(tTmp), "%x", tWords[i]);
	}

	if (tBest.base != -1 && (tBest.base + tBest.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
		*tTp++ = ':';
	*tTp++ = '\0';

	if ((socklen_t)(tTp - tTmp) > pSize)
	{
		errno = ENOSPC;
		return NULL;
	}
	return strcpy(pDst, tTmp);
}

#undef inet_ntop
char *inet_ntop (int pAF, const void *pSrc, char *pDst, socklen_t pSize)
{
	switch (pAF)
	{
		case AF_INET:
			return (inet_ntop4((const u_char*)pSrc, pDst, pSize));
		case AF_INET6:
			return (inet_ntop6((const u_char*)pSrc, pDst, pSize));
		default:
			errno = EINVAL;
			return NULL;
	}
}


#define NS_INADDRSZ		4	/*%< IPv4 T_A */

#undef inet_pton4
int inet_pton4(const char *pSrc, u_char *pDst)
{
	int tSawDigit, tOctets, tChar;
	u_char tTmp[NS_INADDRSZ], *tTp;

	tSawDigit = 0;
	tOctets = 0;
	*(tTp = tTmp) = 0;
	while ((tChar = *pSrc++) != '\0')
	{
		if (tChar >= '0' && tChar <= '9')
		{
			u_int tNew = *tTp * 10 + (tChar - '0');
			if (tSawDigit && *tTp == 0)
				return 0;
			if (tNew > 255)
				return 0;
			*tTp = tNew;
			if (! tSawDigit)
			{
				if (++tOctets > 4)
					return 0;
				tSawDigit = 1;
			}
		} else
		{
			if (tChar == '.' && tSawDigit)
			{
				if (tOctets == 4)
					return 0;
				*++tTp = 0;
				tSawDigit = 0;
			} else
				return 0;
		}
	}
	if (tOctets < 4)
		return 0;
	memcpy(pDst, tTmp, NS_INADDRSZ);
	return 1;
}

#undef inet_pton6
int inet_pton6(const char *pSrc, u_char *pDst)
{
	#ifdef WIN32
		const char tXDigits[] = "0123456789abcdef";
	#else
		static const char tXDigits[] = "0123456789abcdef";
	#endif
	u_char tTmp[NS_IN6ADDRSZ], *tTp, *tEndP, *tColonP;
	const char *tCurrentToken;
	int tChar, tSawXDigit;
	u_int tValue;

	tTp = (u_char*)memset(tTmp, '\0', NS_IN6ADDRSZ);
	tEndP = tTp + NS_IN6ADDRSZ;
	tColonP = NULL;
	if (*pSrc == ':')
		if (*++pSrc != ':')
			return 0;
	tCurrentToken = pSrc;
	tSawXDigit = 0;
	tValue = 0;
	while ((tChar = tolower(*pSrc++)) != '\0')
	{
		const char *pch;
		pch = strchr(tXDigits, tChar);
		if (pch != NULL)
		{
			tValue <<= 4;
			tValue |= (pch - tXDigits);
			if (tValue > 0xffff)
				return 0;
			tSawXDigit = 1;
			continue;
		}
		if (tChar == ':')
		{
			tCurrentToken = pSrc;
			if (!tSawXDigit)
			{
				if (tColonP)
					return 0;
				tColonP = tTp;
				continue;
			} else
			{
				if (*pSrc == '\0')
					return 0;
			}
			if (tTp + NS_INT16SZ > tEndP)
				return 0;
			*tTp++ = (u_char) (tValue >> 8) & 0xff;
			*tTp++ = (u_char) tValue & 0xff;
			tSawXDigit = 0;
			tValue = 0;
			continue;
		}
		if (tChar == '.' && ((tTp + NS_INADDRSZ) <= tEndP) && inet_pton4(tCurrentToken, tTp) > 0)
		{
			tTp += NS_INADDRSZ;
			tSawXDigit = 0;
			break;/* '\0' was seen by inet_pton4(). */
		}
		return 0;
	}
	if (tSawXDigit)
	{
		if (tTp + NS_INT16SZ > tEndP)
			return 0;
		*tTp++ = (u_char) (tValue >> 8) & 0xff;
		*tTp++ = (u_char) tValue & 0xff;
	}
	if (tColonP != NULL)
	{
		const int n = tTp - tColonP;
		int i;

		if (tTp == tEndP)
			return 0;
		for (i = 1; i <= n; i++)
		{
			tEndP[- i] = tColonP[n - i];
			tColonP[n - i] = 0;
		}
		tTp = tEndP;
	}
	if (tTp != tEndP)
		return 0;
	memcpy(pDst, tTmp, NS_IN6ADDRSZ);
	return 1;
}

#undef inet_pton
int inet_pton (int pAF, const char *pSrc, void *pDst)
{
	switch (pAF)
	{
		case AF_INET:
			return (inet_pton4((const char*)pSrc, (u_char*)pDst));
		case AF_INET6:
			return (inet_pton6((const char*)pSrc, (u_char*)pDst));
		default:
			errno = EINVAL;
			return -1;
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////

} //namespace
