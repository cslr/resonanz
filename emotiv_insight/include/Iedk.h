/**
 * Emotiv Development Kit (EDK) API
 * Copyright (c) 2013 Emotiv Systems, Inc.
 *
 * The main interface that allows interactions between external programs and the Emotiv detection engine.
 *
 * None of the API functions are thread-safe.
 *
 * This header file is designed to be includable under C and C++ environment.
 *
 */


#ifndef IEDK_H
#define IEDK_H

#include "IedkErrorCode.h"
#include "IEmoStateDLL.h"


#ifndef EDK_STATIC_LIB

#ifdef EDK_EXPORTS
#ifdef _WIN32
#define EDK_API __declspec(dllexport)
#else
#define EDK_API
#endif
#else
#ifdef _WIN32
#define EDK_API __declspec(dllimport)
#else
#define EDK_API
#endif
#endif

#else

#define EDK_API extern

#endif


#ifdef __cplusplus
extern "C"
{
#endif
	//! FacialExpression Suite threshold type enumerator
	typedef enum IEE_FacialExpressionThreshold_enum {
		FE_SENSITIVITY
	} IEE_FacialExpressionThreshold_t;

	//! FacialExpression Suite training control enumerator
	typedef enum IEE_FacialExpressionTrainingControl_enum {
		FE_NONE = 0, FE_START, FE_ACCEPT, FE_REJECT, FE_ERASE, FE_RESET
	} IEE_FacialExpressionTrainingControl_t;

	//! FacialExpression Suite signature type enumerator
	typedef enum IEE_FacialExpressionSignature_enum {
		FE_SIG_UNIVERSAL = 0, FE_SIG_TRAINED
	} IEE_FacialExpressionSignature_t;

	 //! MentalCommand Suite training control enumerator
	typedef enum IEE_MentalCommandTrainingControl_enum {
		MC_NONE = 0, MC_START, MC_ACCEPT, MC_REJECT, MC_ERASE, MC_RESET
	} IEE_MentalCommandTrainingControl_t;


	//! Handle to internal EmoState structure allocated by IEE_EmoStateCreate()
	typedef void*         EmoStateHandle;

	//! Handle to internal event structure allocated by IEE_EmoEngineEventCreate()
	typedef void*         EmoEngineEventHandle;

	//! Handle to internal event structure allocated by IEE_OptimizationParamCreate()
	typedef void*         OptimizationParamHandle;

	typedef void*		  DataHandle;

	//! EmoEngine event types
	typedef enum IEE_Event_enum {
		IEE_UnknownEvent		= 0x0000,
		IEE_EmulatorError	= 0x0001,
		IEE_ReservedEvent	= 0x0002,
		IEE_UserAdded		= 0x0010,
		IEE_UserRemoved		= 0x0020,
		IEE_EmoStateUpdated	= 0x0040,
		IEE_ProfileEvent		= 0x0080,
		IEE_MentalCommandEvent	= 0x0100,
		IEE_FacialExpressionEvent	= 0x0200,
		IEE_InternalStateChanged = 0x0400,
		IEE_AllEvent			= IEE_UserAdded | IEE_UserRemoved | IEE_EmoStateUpdated | IEE_ProfileEvent |
							  IEE_MentalCommandEvent | IEE_FacialExpressionEvent | IEE_InternalStateChanged
	} IEE_Event_t;

	//! FacialExpression-specific event types
	typedef enum IEE_FacialExpressionEvent_enum {
		IEE_FacialExpressionNoEvent = 0, IEE_FacialExpressionTrainingStarted, IEE_FacialExpressionTrainingSucceeded,
		IEE_FacialExpressionTrainingFailed, IEE_FacialExpressionTrainingCompleted, IEE_FacialExpressionTrainingDataErased,
		IEE_FacialExpressionTrainingRejected, IEE_FacialExpressionTrainingReset
	} IEE_FacialExpressionEvent_t;
	
	//! MentalCommand-specific event types
	typedef enum IEE_MentalCommandEvent_enum {
		IEE_MentalCommandNoEvent = 0, IEE_MentalCommandTrainingStarted, IEE_MentalCommandTrainingSucceeded,
		IEE_MentalCommandTrainingFailed, IEE_MentalCommandTrainingCompleted, IEE_MentalCommandTrainingDataErased,
		IEE_MentalCommandTrainingRejected, IEE_MentalCommandTrainingReset,
		IEE_MentalCommandAutoSamplingNeutralCompleted, IEE_MentalCommandSignatureUpdated
	} IEE_MentalCommandEvent_t;

	typedef enum IEE_DataChannels_enum {
		IED_COUNTER = 0, IED_INTERPOLATED, IED_RAW_CQ,
		IED_AF3, IED_GYROSCOPEZ, IED_GYROSCOPEX, IED_GYROSCOPEY, IED_T7, 
		IED_ACCX, IED_Pz, IED_ACCY, IED_ACCZ, IED_T8, 
		IED_MAGY, IED_MAGZ, IED_MAGX, IED_AF4, IED_GYROX, 
		IED_GYROY, IED_TIMESTAMP, IED_ES_TIMESTAMP, IED_FUNC_ID, IED_FUNC_VALUE, IED_MARKER, 
		IED_SYNC_SIGNAL
	} IEE_DataChannel_t;

	//! Input sensor description
	typedef struct IInputSensorDescriptor_struct {
		IEE_InputChannels_t channelId;  // logical channel id
		int                fExists;    // does this sensor exist on this headset model
		const char*        pszLabel;   // text label identifying this sensor
		double             xLoc;       // x coordinate from center of head towards nose
		double             yLoc;       // y coordinate from center of head towards ears
		double             zLoc;       // z coordinate from center of head toward top of skull
	} IInputSensorDescriptor_t;


	//! Initializes EmoEngine instance which reads EEG data from EPOC. This function should be called at the beginning of programs that make use of EmoEngine, most probably in initialization routine or constructor.
	/*!	
		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if a connection is established

		\sa edkErrorCode.h0
	*/
	EDK_API int
		IEE_EngineConnect(const char* strDevID = "Emotiv Systems-5");

	
	//! Initializes the connection to a remote instance of EmoEngine.
	/*!
		Blocking call

		\param szHost - A null-terminated string identifying the hostname or IP address of the remote EmoEngine server
		\param port - The port number of the remote EmoEngine server
					- If connecting to the Emotiv Control Panel, use port 3008
					- If connecting to the EmoComposer, use port 1726
	
		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if a connection is established

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EngineRemoteConnect(const char* szHost, unsigned short port);


	//! Terminates the connection to EmoEngine. This function should be called at the end of programs which make use of EmoEngine, most probably in clean up routine or destructor.
	/*!
		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if disconnection is achieved

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EngineDisconnect();


	//! Controls the output of logging information from EmoEngine (which is off by default). This should only be enabled if instructed to do so by Emotiv developer support for the purposes of collecting diagnostic information.
	/*!
	    \param szFilename - The path of the logfile
		\param fEnable - If non-zero, then diagnostic information will be written to logfile.
		               - If zero, then all diagnostic information is suppressed (default).
	    \param nReserved - Reserved for future use.

	    \return EDK_ERROR_CODE
		        - EDK_ERROR_CODE = EDK_OK if the command succeeded
	*/
	EDK_API int
		IEE_EnableDiagnostics(const char* szFilename, int fEnable, int nReserved);

	
	//! Returns a handle to memory that can hold an EmoEngine event. This handle can be reused by the caller to retrieve subsequent events.
	/*!
		\return EmoEngineEventHandle
	*/
	EDK_API EmoEngineEventHandle
		IEE_EmoEngineEventCreate();


	//! Returns a handle to memory that can hold a profile byte stream. This handle can be reused by the caller to retrieve subsequent profile bytes.
	/*!
		\return EmoEngineEventHandle
	*/
	EDK_API EmoEngineEventHandle
		IEE_ProfileEventCreate();

	
	//! Frees memory referenced by an event handle.
	/*!
		\param hEvent - a handle returned by IEE_EmoEngineEventCreate() or IEE_ProfileEventCreate()
	*/
	EDK_API void
		IEE_EmoEngineEventFree(EmoEngineEventHandle hEvent);

	
	//! Returns a handle to memory that can store an EmoState. This handle can be reused by the caller to retrieve subsequent EmoStates.
	/*!
		\return EmoStateHandle
	*/
	EDK_API EmoStateHandle
		IEE_EmoStateCreate();

	
	//! Frees memory referenced by an EmoState handle.
	/*!
		\param hState - a handle returned by IEE_EmoStateCreate()
	*/
	EDK_API void
		IEE_EmoStateFree(EmoStateHandle hState);


	//! Returns the event type for an event already retrieved using IEE_EngineGetNextEvent.
	/*!
		\param hEvent - a handle returned by IEE_EmoEngineEventCreate()
	
		\return IEE_Event_t
	*/
	EDK_API IEE_Event_t
		IEE_EmoEngineEventGetType(EmoEngineEventHandle hEvent);

	
	//! Returns the MentalCommand-specific event type for an IEE_MentalCommandEvent event already retrieved using IEE_EngineGetNextEvent.
	/*!
		\param hEvent - a handle returned by IEE_EmoEngineEventCreate()
	
		\return IEE_MentalCommandEvent_t
	*/
	EDK_API IEE_MentalCommandEvent_t
		IEE_MentalCommandEventGetType(EmoEngineEventHandle hEvent);


	//! Returns the FacialExpression-specific event type for an IEE_FacialExpressionEvent event already retrieved using IEE_EngineGetNextEvent.
	/*!
		\param hEvent - a handle returned by IEE_EmoEngineEventCreate()
	
		\return IEE_FacialExpressionEvent_t
	*/
	EDK_API IEE_FacialExpressionEvent_t
		IEE_FacialExpressionEventGetType(EmoEngineEventHandle hEvent);
	

	//! Retrieves the user ID for IEE_UserAdded and IEE_UserRemoved events.
	/*!
		\param hEvent - a handle returned by IEE_EmoEngineEventCreate()
		\param pUserIdOut - receives the user ID associated with the current event
	
		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful.

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EmoEngineEventGetUserId(EmoEngineEventHandle hEvent, unsigned int *pUserIdOut);

	
	//! Copies an EmoState returned with a IEE_EmoStateUpdate event to memory referenced by an EmoStateHandle.
	/*!
		\param hEvent - a handle returned by IEE_EmoEngineEventCreate() and populated with IEE_EmoEngineGetNextEvent()
		\param hEmoState - a handle returned by IEE_EmoStateCreate
	
		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful.

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EmoEngineEventGetEmoState(EmoEngineEventHandle hEvent, EmoStateHandle hEmoState);
	

	//! Retrieves the next EmoEngine event
	/*!
		Non-blocking call

		\param hEvent - a handle returned by IEE_EmoEngineEventCreate()

		\return EDK_ERROR_CODE
                <ul>
		        <li> EDK_ERROR_CODE = EDK_OK if a new event has been retrieved
				<li> EDK_ERROR_CODE = EDK_NO_EVENT if no new events have been generated by EmoEngine
				</ul>
		
		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EngineGetNextEvent(EmoEngineEventHandle hEvent);

	
	//! Clear a specific EmoEngine event type or all events currently inside the event queue. Event flags can be combined together as one argument except IEE_UnknownEvent and IEE_EmulatorError.
	/*!
		\param eventTypes - EmoEngine event type (IEE_Event_t), multiple events can be combined such as (IEE_UserAdded | IEE_UserRemoved)

		\return EDK_ERROR_CODE
                <ul>
		        <li> EDK_ERROR_CODE = EDK_OK if the events have been cleared from the queue
				<li> EDK_ERROR_CODE = EDK_INVALID_PARAMETER if input event types are invalid
				</ul>
		
		\sa IEE_Event_t, edkErrorCode.h
	*/
	EDK_API int
		IEE_EngineClearEventQueue(int eventTypes);

	
	//! Retrieves number of active users connected to the EmoEngine.
	/*!
		\param pNumUserOut - receives number of users

		\return EDK_ERROR_CODE
		        - EDK_ERROR_CODE = EDK_OK if successful.

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EngineGetNumUser(unsigned int* pNumUserOut);


	//! Sets the player number displayed on the physical input device (currently the USB Dongle) that corresponds to the specified user
	/*!
		\param userId - EmoEngine user ID
		\param playerNum - application assigned player number displayed on input device hardware (must be in the range 1-4)
		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_SetHardwarePlayerDisplay(unsigned int userId, unsigned int playerNum);


	//! Loads an EmoEngine profile for the specified user.  
	/*!
		\param userId - user ID
		\param profileBuffer - pointer to buffer containing a serialized user profile previously returned from EmoEngine.
		\param length - buffer size (number of bytes)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE if the function succeeds in loading this profile

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_SetUserProfile(unsigned int userId, const unsigned char profileBuffer[], unsigned int length);


	//! Returns user profile data in a synchronous manner.
	/*!
	     Fills in the event referred to by hEvent with an IEE_ProfileEvent event
		 that contains the profile data for the specified user.

		 \param userId - user ID
		 \param hEvent - a handle returned by IEE_EmoEngineEventCreate()

		 \return EDK_ERROR_CODE
		         - EDK_ERROR_CODE = EDK_OK if successful

		 \sa edkErrorCode.h
	*/
	EDK_API int
		IEE_GetUserProfile(unsigned int userId, EmoEngineEventHandle hEvent);


	//! Returns a serialized user profile for a default user in a synchronous manner.
	/*!
	    Fills in the event referred to by hEvent with an IEE_ProfileEvent event
		that contains the profile data for the default user

		 \param hEvent - a handle returned by IEE_EmoEngineEventCreate()

		 \return EDK_ERROR_CODE
		         - EDK_ERROR_CODE = EDK_OK if successful

		 \sa edkErrorCode.h
	*/
	EDK_API int
		IEE_GetBaseProfile(EmoEngineEventHandle hEvent);
	

	//! Returns the number of bytes required to store a serialized version of the requested user profile.
	/*!	
		\param hEvt - an EmoEngineEventHandle of type IEE_ProfileEvent
		\param pProfileSizeOut - receives number of bytes required by the profile

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_GetUserProfileSize(EmoEngineEventHandle hEvt, unsigned int* pProfileSizeOut);

	
	//! Copies a serialized version of the requested user profile into the caller's buffer.
	/*!		
		\param hEvt - an EmoEngineEventHandle returned in a IEE_ProfileEvent event
		\param destBuffer - pointer to a destination buffer
		\param length - the size of the destination buffer in bytes

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_GetUserProfileBytes(EmoEngineEventHandle hEvt, unsigned char destBuffer[], unsigned int length);

	//!  Loads a user profile from disk and assigns it to the specified user
	/*!		
		\param userID - a valid user ID
		\param szInputFilename - platform-dependent filesystem path of saved user profile

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_LoadUserProfile(unsigned int userID, const char* szInputFilename);

	//!  Saves a user profile for specified user to disk
	/*!		
		\param userID - a valid user ID
		\param szOutputFilename - platform-dependent filesystem path for output file

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_SaveUserProfile(unsigned int userID, const char* szOutputFilename);

	//! Set threshold for FacialExpression algorithms
	/*!
		\param userId - user ID
		\param algoName - FacialExpression algorithm type
		\param thresholdName - FacialExpression threshold type
		\param value - threshold value (min: 0 max: 1000)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_FacialExpressionAlgo_t, IEE_FacialExpressionThreshold_t
	*/
	EDK_API int
		IEE_FacialExpressionSetThreshold(unsigned int userId, IEE_FacialExpressionAlgo_t algoName, IEE_FacialExpressionThreshold_t thresholdName, int value);


	//! Get threshold from FacialExpression algorithms
	/*!
		\param userId - user ID
		\param algoName - FacialExpression algorithm type
		\param thresholdName - FacialExpression threshold type
		\param pValueOut - receives threshold value

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_FacialExpressionAlgo_t, IEE_FacialExpressionThreshold_t
	*/
	EDK_API int
		IEE_FacialExpressionGetThreshold(unsigned int userId, IEE_FacialExpressionAlgo_t algoName, IEE_FacialExpressionThreshold_t thresholdName, int *pValueOut);


	//! Set the current facial expression for FacialExpression training
	/*!
		Blocking call

		\param userId - user ID
		\param action - which facial expression would like to be trained

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa edkErrorCode.h, IEE_FacialExpressionAlgo_t
	*/
	EDK_API int 
		IEE_FacialExpressionSetTrainingAction(unsigned int userId, IEE_FacialExpressionAlgo_t action);


	//! Set the control flag for FacialExpression training
	/*!
		Blocking call

		\param userId - user ID
		\param control - pre-defined control command

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa edkErrorCode.h, IEE_FacialExpressionTrainingControl_t
	*/
	EDK_API int 
		IEE_FacialExpressionSetTrainingControl(unsigned int userId, IEE_FacialExpressionTrainingControl_t control);


	//! Gets the facial expression currently selected for FacialExpression training
	/*!
		Blocking call

		\param userId - user ID
		\param pActionOut - receives facial expression currently selected for training

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa EDK_ERROR_CODE, IEE_FacialExpressionAlgo_t
	*/
	EDK_API int 
		IEE_FacialExpressionGetTrainingAction(unsigned int userId, IEE_FacialExpressionAlgo_t* pActionOut);

	
	//! Return the duration of a FacialExpression training session
	/*!
		\param userId - user ID
		\param pTrainingTimeOut - receive the training time in ms

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int 
		IEE_FacialExpressionGetTrainingTime(unsigned int userId, unsigned int* pTrainingTimeOut);


	//! Gets a list of the actions that have been trained by the user
	/*!
		Blocking call

		\param userId - user ID
		\param pTrainedActionsOut - receives a bit vector composed of IEE_FacialExpressionAlgo_t contants

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa EDK_ERROR_CODE, IEE_FacialExpressionAlgo_t
	*/
    EDK_API int
        IEE_FacialExpressionGetTrainedSignatureActions(unsigned int userId, unsigned long* pTrainedActionsOut);


	//! Gets a flag indicating if the user has trained sufficient actions to activate a trained signature
	/*!
        *pfAvailableOut will be set to 1 if the user has trained FE_NEUTRAL and at least
        one other FacialExpression action.  Otherwise, *pfAvailableOut == 0.

		Blocking call

		\param userId - user ID
		\param pfAvailableOut - receives an int that is non-zero if a trained signature can be activated

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa EDK_ERROR_CODE
	*/
    EDK_API int
		IEE_FacialExpressionGetTrainedSignatureAvailable(unsigned int userId, int* pfAvailableOut);

	//! Configures the FacialExpression suite to use either the built-in, universal signature or a personal, trained signature
	/*!
        Note: FacialExpression defaults to use its universal signature.  This function will fail if IEE_FacialExpressionGetTrainedSignatureAvailable returns false.

		Blocking call

		\param userId - user ID
		\param sigType - signature type to use

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa EDK_ERROR_CODE, IEE_FacialExpressionSignature_t
	*/
	EDK_API int 
		IEE_FacialExpressionSetSignatureType(unsigned int userId, IEE_FacialExpressionSignature_t sigType);

	//! Indicates whether the FacialExpression suite is currently using either the built-in, universal signature or a trained signature
	/*!
		Blocking call

		\param userId - user ID
		\param pSigTypeOut - receives the signature type currently in use

		\return EDK_ERROR_CODE - current status of EmoEngine. If the query is successful, EDK_ERROR_CODE = OK.

		\sa EDK_ERROR_CODE, IEE_FacialExpressionSignature_t
	*/
	EDK_API int 
		IEE_FacialExpressionGetSignatureType(unsigned int userId, IEE_FacialExpressionSignature_t* pSigTypeOut);




	//! Set the current MentalCommand active action types
	/*!
		\param userId - user ID
		\param activeActions - a bit vector composed of IEE_MentalCommandAction_t contants

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandAction_t
	*/
	EDK_API int
		IEE_MentalCommandSetActiveActions(unsigned int userId, unsigned long activeActions);

	
	//! Get the current MentalCommand active action types
	/*!
		\param userId - user ID
		\param pActiveActionsOut - receive a bit vector composed of IEE_MentalCommandAction_t contants

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandAction_t
	*/
	EDK_API int
		IEE_MentalCommandGetActiveActions(unsigned int userId, unsigned long* pActiveActionsOut);

	
	//! Return the duration of a MentalCommand training session
	/*!
		\param userId - user ID
		\param pTrainingTimeOut - receive the training time in ms

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int 
		IEE_MentalCommandGetTrainingTime(unsigned int userId, unsigned int* pTrainingTimeOut);

	
	//! Set the training control flag for MentalCommand training
	/*!
		\param userId - user ID
		\param control - pre-defined MentalCommand training control

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandTrainingControl_t
	*/
	EDK_API int 
		IEE_MentalCommandSetTrainingControl(unsigned int userId, IEE_MentalCommandTrainingControl_t control);

	
	//! Set the type of MentalCommand action to be trained
	/*!
		\param userId - user ID
		\param action - which action would like to be trained

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandAction_t
	*/
	EDK_API int 
		IEE_MentalCommandSetTrainingAction(unsigned int userId, IEE_MentalCommandAction_t action);


	//! Get the type of MentalCommand action currently selected for training
	/*!
		\param userId - user ID
		\param pActionOut - action that is currently selected for training

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandAction_t
	*/
	EDK_API int 
		IEE_MentalCommandGetTrainingAction(unsigned int userId, IEE_MentalCommandAction_t* pActionOut);


	//! Gets a list of the MentalCommand actions that have been trained by the user
	/*!
		Blocking call

		\param userId - user ID
		\param pTrainedActionsOut - receives a bit vector composed of IEE_MentalCommandAction_t contants

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandAction_t
	*/
    EDK_API int
        IEE_MentalCommandGetTrainedSignatureActions(unsigned int userId, unsigned long* pTrainedActionsOut);
	
	
	//! Gets the current overall skill rating of the user in MentalCommand
	/*!
		Blocking call

		\param userId - user ID
		\param pOverallSkillRatingOut - receives the overall skill rating [from 0.0 to 1.0]

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
    EDK_API int
        IEE_MentalCommandGetOverallSkillRating(unsigned int userId, float* pOverallSkillRatingOut);


	//! Gets the current skill rating for particular MentalCommand actions of the user
	/*!
		Blocking call

		\param userId - user ID
		\param action - a particular action of IEE_MentalCommandAction_t contant
		\param pActionSkillRatingOut - receives the action skill rating [from 0.0 to 1.0]

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h, IEE_MentalCommandAction_t
	*/
    EDK_API int
        IEE_MentalCommandGetActionSkillRating(unsigned int userId, IEE_MentalCommandAction_t action, float* pActionSkillRatingOut);

	
	//! Set the overall sensitivity for all MentalCommand actions
	/*!
		\param userId - user ID
		\param level - sensitivity level of all actions (lowest: 1, highest: 7)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int 
		IEE_MentalCommandSetActivationLevel(unsigned int userId, int level);

	
	//! Set the sensitivity of MentalCommand actions
	/*!
		\param userId - user ID
		\param action1Sensitivity - sensitivity of action 1 (min: 1, max: 10)
		\param action2Sensitivity - sensitivity of action 2 (min: 1, max: 10)
		\param action3Sensitivity - sensitivity of action 3 (min: 1, max: 10)
		\param action4Sensitivity - sensitivity of action 4 (min: 1, max: 10)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int 
		IEE_MentalCommandSetActionSensitivity(unsigned int userId,
										int action1Sensitivity, int action2Sensitivity,
										int action3Sensitivity, int action4Sensitivity);

	
	//! Get the overall sensitivity for all MentalCommand actions
	/*!
		\param userId - user ID
		\param pLevelOut - sensitivity level of all actions (min: 1, max: 10)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int 
		IEE_MentalCommandGetActivationLevel(unsigned int userId, int *pLevelOut);

	
	//! Query the sensitivity of MentalCommand actions
	/*!
		\param userId - user ID
		\param pAction1SensitivityOut - sensitivity of action 1
		\param pAction2SensitivityOut - sensitivity of action 2
		\param pAction3SensitivityOut - sensitivity of action 3
		\param pAction4SensitivityOut - sensitivity of action 4

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int 
		IEE_MentalCommandGetActionSensitivity(unsigned int userId,
										int* pAction1SensitivityOut, int* pAction2SensitivityOut,
										int* pAction3SensitivityOut, int* pAction4SensitivityOut);

	
	//! Start the sampling of Neutral state in MentalCommand
	/*!
		\param userId - user ID

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_MentalCommandStartSamplingNeutral(unsigned int userId);

	
	//! Stop the sampling of Neutral state in MentalCommand
	/*!
		\param userId - user ID

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_MentalCommandStopSamplingNeutral(unsigned int userId);

	
	//! Enable or disable signature caching in MentalCommand
	/*!
		\param userId  - user ID
		\param enabled - flag to set status of caching (1: enable, 0: disable)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_MentalCommandSetSignatureCaching(unsigned int userId, unsigned int enabled);


	//! Query the status of signature caching in MentalCommand
	/*!
		\param userId  - user ID
		\param pEnabledOut - flag to get status of caching (1: enable, 0: disable)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_MentalCommandGetSignatureCaching(unsigned int userId, unsigned int* pEnabledOut);


	//! Set the cache size for the signature caching in MentalCommand
	/*!
		\param userId - user ID
		\param size   - number of signatures to be kept in the cache (0: unlimited)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_MentalCommandSetSignatureCacheSize(unsigned int userId, unsigned int size);


	//! Get the current cache size for the signature caching in MentalCommand
	/*!
		\param userId - user ID
		\param pSizeOut - number of signatures to be kept in the cache (0: unlimited)

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if successful

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_MentalCommandGetSignatureCacheSize(unsigned int userId, unsigned int* pSizeOut);


	//! Returns a struct containing details about the specified EEG channel's headset 
    /*!
        \param channelId - channel identifier (see EmoStateDll.h)
        \param pDescriptorOut - provides detailed sensor location and other info

        \return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful

        \sa EmoStateDll.h, edkErrorCode.h
	*/
	EDK_API int
		IEE_HeadsetGetSensorDetails(IEE_InputChannels_t channelId, IInputSensorDescriptor_t* pDescriptorOut);


	//! Returns the current hardware version of the headset and dongle for a particular user
    /*!
        \param userId - user ID for query
		\param pHwVersionOut - hardware version for the user headset/dongle pair. hiword is headset version, loword is dongle version.

        \return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful

        \sa EmoStateDll.h, edkErrorCode.h
	*/
	EDK_API int
		IEE_HardwareGetVersion(unsigned int userId, unsigned long* pHwVersionOut);

	//! Returns the current version of the Emotiv SDK software
    /*!
		\param pszVersionOut - SDK software version in X.X.X.X format. Note: current beta releases have a major version of 0.
		\param nVersionChars - Length of char buffer pointed to by pszVersion argument.
		\param pBuildNumOut  - Build number.  Unique for each release.

        \return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful

        \sa edkErrorCode.h
	*/
	EDK_API int
		IEE_SoftwareGetVersion(char* pszVersionOut, unsigned int nVersionChars, unsigned long* pBuildNumOut);

	//! Returns the delta of the movement of the gyro since the previous call for a particular user
	/*!
		\param userId - user ID for query
		\param pXOut  - horizontal displacement
		\param pYOut  - vertical displacment

		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful

        \sa EmoStateDll.h
        \sa edkErrorCode.h
	*/
	EDK_API int
		IEE_HeadsetGetGyroDelta(unsigned int userId, int* pXOut, int* pYOut);

	//! Re-zero the gyro for a particular user
	/*!
		\param userId - user ID for query

		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful

        \sa EmoStateDll.h
        \sa edkErrorCode.h
	*/
	EDK_API int
		IEE_HeadsetGyroRezero(unsigned int userId);

	//! Returns a handle to memory that can hold an optimization paramaeter which is used to configure the behaviour of optimization
	/*!
		\return OptimizationParamHandle
	*/
	EDK_API OptimizationParamHandle
		IEE_OptimizationParamCreate();

	//! Frees memory referenced by an optimization parameter handle
	/*!
		\param hParam - a handle returned by IEE_OptimizationParamCreate()
	*/
	EDK_API void
		IEE_OptimizationParamFree(OptimizationParamHandle hParam);

	//! Enable optimization. EmoEngine will try to optimize its performance according to the information passed in with optimization parameter. EmoEngine guarantees the correctness of the results of vital algorithms. For algorithms that are not vital, results are undefined.
	/*!
		\param hParam - a handle returned by IEE_OptimizationParamCreate()
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
	EDK_API int
		IEE_OptimizationEnable(OptimizationParamHandle hParam);

	//! Determine whether optimization is on
	/*!
		\param pEnabledOut - receives information about whether optimization is on
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
	EDK_API int
		IEE_OptimizationIsEnabled(bool* pEnabledOut);

	//! Disable optimization
	/*!
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
	EDK_API int
		IEE_OptimizationDisable();

	//! Get optimization parameter.  If optimization is not enabled (this can be checked with IEE_OptimmizationIsEnabled) then the results attached to the hParam parameter are undefined.
	/*!
		\param hParam - a handle returned by IEE_OptimizationParamCreate()
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
	EDK_API int
		IEE_OptimizationGetParam(OptimizationParamHandle hParam);

	//! Get a list of vital algorithms of specific suite from optimization parameter
	/*!
		\param hParam - a handle returned by IEE_OptimizationParamCreate()
		\param suite - suite that you are interested in
		\param pVitalAlgorithmBitVectorOut - receives a list of vital algorithm composed of IEE_FacialExpressionAlgo_t, IEE_PerformanceMetricAlgo_t or IEE_MentalCommandAction_t depending on the suite parameter
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
	EDK_API int
		IEE_OptimizationGetVitalAlgorithm(OptimizationParamHandle hParam, IEE_EmotivSuite_t suite, unsigned int* pVitalAlgorithmBitVectorOut);

	//! Set a list of vital algorithms of specific suite to optimization parameter
	/*!
		\param hParam - a handle returned by IEE_OptimizationParamCreate()
		\param suite - suite that you are interested in
		\param vitalAlgorithmBitVector - a list of vital algorithm composed of IEE_FacialExpressionAlgo_t, IEE_PerformanceMetricAlgo_t or IEE_MentalCommandAction_t depended on the suite parameter passed in
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
	EDK_API int
		IEE_OptimizationSetVitalAlgorithm(OptimizationParamHandle hParam, IEE_EmotivSuite_t suite, unsigned int vitalAlgorithmBitVector);

	//! Resets all settings and user-specific profile data for the specified detection suite
	/*!
		\param userId - user ID
		\param suite - detection suite (FacialExpression, PerformanceMetric, or MentalCommand)
		\param detectionBitVector - identifies specific detections.  Set to zero for all detections.
		\return EDK_ERROR_CODE
                - EDK_ERROR_CODE = EDK_OK if successful
	*/
    EDK_API int IEE_ResetDetection(unsigned int userId, IEE_EmotivSuite_t suite, unsigned int detectionBitVector);
		/*
	Check Customer ID code
	*/
	EDK_API double  IEE_GetSecurityCode(void);//get id code from EDK
	EDK_API bool IEE_CheckSecurityCode(double x); //check your Id code

	//! Initializes EmoEngine instance which reads EEG data from a pre-recorded session file.
	/*!	
		\param szHost - File path to the pre-recorded EEG file.

		\return EDK_ERROR_CODE
				- EDK_ERROR_CODE = EDK_OK if an instance is created successfully

		\sa edkErrorCode.h
	*/
	EDK_API int
		IEE_EngineLocalConnect(const char* strFilePath);


#ifdef __cplusplus
};
#endif
#endif
