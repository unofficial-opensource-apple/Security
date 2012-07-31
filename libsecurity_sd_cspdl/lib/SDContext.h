/*
 * Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


//
// SDContext.h - Security Server contexts 
//
#ifndef _H_SD_CONTEXT
#define _H_SD_CONTEXT

#include <security_cdsa_plugin/CSPsession.h>
#include <securityd_client/ssclient.h>
#include <security_cdsa_utilities/digestobject.h>
#include <security_cdsa_client/cspclient.h>

//
// Parent class for all CSPContexts implemented in this CSP.  Currently the
// only thing we add is a reference to our creator's session.
//
class SDCSPSession;
class SDKey;

class SDContext : public CSPFullPluginSession::CSPContext
{
public:
	SDContext(SDCSPSession &session);
	~SDContext() { clearOutBuf(); }
	virtual void init(const Context &context, bool encoding);

protected:
	SecurityServer::ClientSession &clientSession();
	SDCSPSession &mSession;
	
	// mOutBuf provides a holding tank for implied final() operations
	// resulting from an outputSize(true, 0). This form of outputSize()
	// is understood to only occur just prior to the final() call. To avoid
	// an extra RPC (just to perform the outputSize(), most subclasses of
	// SDContext actually perform the final() operation at this time,
	// storing the result in mOutBuf. At final(), mOutBuf() is just copied
	// to the caller's supplied output buffer. 
	CssmData mOutBuf;		
	
	// We remember a pointer to the passed in context and assume it will
	// remain a valid from init(), update() all the way though the call to
	// final().
	const Context *mContext;
	
	void clearOutBuf();
	void copyOutBuf(CssmData &out);
};

// context for signature (sign and verify)
class SDSignatureContext : public SDContext
{
public:
	SDSignatureContext(SDCSPSession &session);
	~SDSignatureContext();
	virtual void init(const Context &context, bool signing);
	virtual void update(const CssmData &data);
	virtual size_t outputSize(bool final, size_t inSize);
	
	/* sign */
	void sign(CssmData &sig);
	virtual void final(CssmData &out);
	
	/* verify */
	virtual void final(const CssmData &in);
	
	/* for raw sign/verify - optionally called after init */ 
	virtual void setDigestAlgorithm(CSSM_ALGORITHMS digestAlg);

private:
	/* stash the context's key for final sign/verify */
	SecurityServer::KeyHandle mKeyHandle;	
	
	/* alg-dependent, calculated at init time */
	CSSM_ALGORITHMS	mSigAlg;		// raw signature alg
	CSSM_ALGORITHMS mDigestAlg;		// digest
	CSSM_ALGORITHMS mOrigAlg;		// caller's context alg
	
	/* exactly one of these is used to collect updates */
	NullDigest 			*mNullDigest;
	CssmClient::Digest 	*mDigest;
};

// Context for GenerateRandom operations
class SDRandomContext : public SDContext
{
public:
	SDRandomContext(SDCSPSession &session);
	virtual void init(const Context &context, bool);
	virtual size_t outputSize(bool final, size_t inSize);
	virtual void final(CssmData &out);
	
private:
	uint32 mOutSize;		// spec'd in context at init() time 
};

// Context for Encrypt and Decrypt operations
class SDCryptContext : public SDContext
{
public:
	SDCryptContext(SDCSPSession &session);
	~SDCryptContext();
	virtual void init(const Context &context, bool encoding);
	virtual size_t inputSize(size_t outSize);
	virtual size_t outputSize(bool final, size_t inSize);
	virtual void minimumProgress(size_t &in, size_t &out);
	virtual void update(void *inp, size_t &inSize, void *outp,
						size_t &outSize);
	virtual void final(CssmData &out);

private:
	SecurityServer::KeyHandle mKeyHandle;
	NullDigest mNullDigest;						// accumulator
};

// Digest, using raw CSP
class SDDigestContext : public SDContext
{
public:
	SDDigestContext(SDCSPSession &session);
	~SDDigestContext();
	virtual void init(const Context &context, bool);
	virtual void update(const CssmData &data);
	virtual void final(CssmData &out);
	virtual size_t outputSize(bool final, size_t inSize);

private:
	CssmClient::Digest *mDigest;
};

// common class for MAC generate, verify
class SDMACContext : public SDContext
{
public:
	SDMACContext(SDCSPSession &session);
	virtual void init(const Context &context, bool);
	virtual void update(const CssmData &data);
	virtual size_t outputSize(bool final, size_t inSize);
	
	/* sign */
	void genMac(CssmData &mac);
	virtual void final(CssmData &out);
	/* verify */
	virtual void final(const CssmData &in);
	
private:
	SecurityServer::KeyHandle mKeyHandle;
	NullDigest mNullDigest;					// accumulator
};


#endif // _H_SD_CONTEXT
