/**
 * Emotiv SDK
 * Copyright (c) 2016 Emotiv Inc.
 *
 * This file is part of the Emotiv SDK.
 *
 * All the error codes that are returned by EC_ functions are listed here.
 */
//! \file

#ifndef EC_ERROR_CODE_H
#define EC_ERROR_CODE_H

//! Default success value.
#define EC_OK                               0x0000

//! An internal error occurred.
#define EC_UNKNOWN_ERROR                    0x0001

//! An internal error occurred.
#define EC_COULDNT_RESOLVE_PROXY            0x0002

#define EC_COULDNT_RESOLVE_HOST             0x0003

#define EC_COULDNT_CONNECT                  0x0004

//! Profile created by EC_SaveUserProfile() is existed in Emotiv Cloud.
#define EC_PROFILE_CLOUD_EXISTED            0x0101

//! The buffer is not a valid, serialized EmoEngine profile.
#define EC_INVALID_PROFILE_ARCHIVE          0x0102

//! One of the parameters supplied to the function is invalid
#define EC_INVALID_PARAMETER                0x0300

//! A parameter supplied to the function is out of range.
#define EC_OUT_OF_RANGE                     0x0301

//! A filesystem error occurred.
#define EC_FILESYSTEM_ERROR                 0x0302

//! The buffer supplied to the function is not large enough.
#define EC_BUFFER_TOO_SMALL                 0x0303

//! The file uploaded to cloud is failed
#define EC_UPLOAD_FAILED                    0x0304

//! The cloud user ID supplied to the function is invalid.
#define EC_INVALID_CLOUD_USER_ID            0x0400

//! The user ID supplied to the function is invalid
#define EC_INVALID_ENGINE_USER_ID           0x0401

//! The user ID supplied to the function dont login, call EC_Login() first
#define EC_CLOUD_USER_ID_DONT_LOGIN         0x0402

//! The Emotiv Cloud needs to be initialized via EC_Connect()
#define EC_EMOTIVCLOUD_UNINITIALIZED        0x0500

//! The Emotiv Engine needs to be initialized via IEE_EngineConnect() or IEE_EngineRemoteConnect() before call EC_Connect() or Emotiv Engine is disconnected
#define EC_EMOENGINE_UNINITIALIZED          0x0501


#define EC_FILE_EXISTS                      0x1001	

//! Reserved return value.
#define EC_RESERVED1                        0x0900

#endif
