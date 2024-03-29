/*
** Copyright 2022 bitValence, Inc.
** All Rights Reserved.
**
** This program is free software; you can modify and/or redistribute it
** under the terms of the GNU Affero General Public License
** as published by the Free Software Foundation; version 3 with
** attribution addendums as found in the LICENSE.txt
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Affero General Public License for more details.
**
** Purpose:
**   Manage MQTT topic table
**
** Notes:
**   1. The JSON topic table defines each supported MQTT topic. Topics
**      are
**      - Read from MQTT broker, translated from JSOn to binary cFE
**        message and published on the SB
**      - Read from the SB, translated from binary cFE message to
**        JSON and sent to an MQTT broker
**   2. See mqtt_gw/fsw/topics/mqtt_gw_topics.c for topic plugin instructions 
**
*/

#ifndef _mqtt_topic_tbl_
#define _mqtt_topic_tbl_

/*
** Includes
*/

#include "app_cfg.h"
#include "mqtt_topic_tbl.h"
#include "mqtt_gw_eds_defines.h"

/***********************/
/** Macro Definitions **/
/***********************/


#define JSON_MAX_KW_LEN  10  // Maximum length of short keywords

/*
** Event Message IDs
*/

#define MQTT_TOPIC_TBL_INDEX_ERR_EID  (MQTT_TOPIC_TBL_BASE_EID + 0)
#define MQTT_TOPIC_TBL_DUMP_ERR_EID   (MQTT_TOPIC_TBL_BASE_EID + 1)
#define MQTT_TOPIC_TBL_LOAD_ERR_EID   (MQTT_TOPIC_TBL_BASE_EID + 2)
#define MQTT_TOPIC_TBL_STUB_EID       (MQTT_TOPIC_TBL_BASE_EID + 3)
#define MQTT_TOPIC_TBL_ENA_PLUGIN_EID (MQTT_TOPIC_TBL_BASE_EID + 4)
#define MQTT_TOPIC_TBL_DIS_PLUGIN_EID (MQTT_TOPIC_TBL_BASE_EID + 5)

/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Table - Local table copy used for table loads
** 
*/

typedef struct
{

   char    Mqtt[MQTT_GW_MAX_MQTT_TOPIC_LEN];
   uint16  Cfe;
   char    SbStr[JSON_MAX_KW_LEN];
   char    EnaStr[JSON_MAX_KW_LEN];
   
   // These are set based on JSON strings
   bool    Enabled;
   MQTT_GW_TopicSbRole_Enum_t SbRole;
   
} MQTT_TOPIC_TBL_Topic_t;

typedef struct
{

  MQTT_TOPIC_TBL_Topic_t Topic[MQTT_GW_TopicPlugin_Enum_t_MAX];
   
} MQTT_TOPIC_TBL_Data_t;


/******************************************************************************
** Topic 'virtual' function signatures
** - Separate MQTT_TOPIC_xxx files are used for each topic plugin. The
**   MQTT_TOPIC_TBL_PluginFuncTbl_t is used to hold pointers to each conversion
**   function so you can think of a plugin type as an abstract base class and
**   each MQTT_TOPIC_xxx as concrete classes that provide the plugin conversion
**   methods.
*/

typedef bool (*MQTT_TOPIC_TBL_JsonToCfe_t)(CFE_MSG_Message_t **CfeMsg, const char *JsonMsgPayload, uint16 PayloadLen);
typedef bool (*MQTT_TOPIC_TBL_CfeToJson_t)(const char **JsonMsgPayload, const CFE_MSG_Message_t *CfeMsg);
typedef void (*MQTT_TOPIC_TBL_SbMsgTest_t)(bool Init, int16 Param);

typedef struct
{

   MQTT_TOPIC_TBL_CfeToJson_t  CfeToJson;
   MQTT_TOPIC_TBL_JsonToCfe_t  JsonToCfe;  
   MQTT_TOPIC_TBL_SbMsgTest_t  SbMsgTest;

} MQTT_TOPIC_TBL_PluginFuncTbl_t; 


/******************************************************************************
** Class
*/

typedef struct
{

   /*
   ** Topic Table
   */
   
   MQTT_TOPIC_TBL_Data_t  Data;
   
   /*
   ** Standard CJSON table data
   */
   
   uint32      DiscreteTlmTopicId;
   bool        Loaded;   /* Has entire table been loaded? */
   uint16      LastLoadCnt;
   
   size_t      JsonObjCnt;
   char        JsonBuf[MQTT_TOPIC_TBL_JSON_FILE_MAX_CHAR];   
   size_t      JsonFileLen;
   
} MQTT_TOPIC_TBL_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: MQTT_TOPIC_TBL_Constructor
**
** Initialize the MQTT topic table.
**
** Notes:
**   1. The table values are not populated. This is done when the table is 
**      registered with the table manager.
**
*/
void MQTT_TOPIC_TBL_Constructor(MQTT_TOPIC_TBL_Class_t *MqttTopicTblPtr,
                                uint32 DiscreteTlmTopicId);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_DisablePlugin
**
** Disable a topic plugin.
** 
** Notes:
**   1. Sends events for errors and not for success.
**
*/
bool MQTT_TOPIC_TBL_DisablePlugin(enum MQTT_GW_TopicPlugin TopicPlugin);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_DumpCmd
**
** Command to write the table data from memory to a JSON file.
**
** Notes:
**  1. Function signature must match TBLMGR_DumpTblFuncPtr_t.
**
*/
bool MQTT_TOPIC_TBL_DumpCmd(osal_id_t  FileHandle);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_EnablePlugin
**
** Enable a topic plugin.
** 
** Notes:
**   1. Sends events for errors and not for success.
**
*/
bool MQTT_TOPIC_TBL_EnablePlugin(enum MQTT_GW_TopicPlugin TopicPlugin);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_GetCfeToJson
**
** Return a pointer to the CfeToJson conversion function for 'TopicPlugin'
** and return a pointer to the JSON topic string in JsonMsgTopic.
** 
** Notes:
**   1. TopicPlugin must be less than MQTT_GW_TopicPlugin_Enum_t_MAX
**
*/
MQTT_TOPIC_TBL_CfeToJson_t MQTT_TOPIC_TBL_GetCfeToJson(enum MQTT_GW_TopicPlugin TopicPlugin,
                                                       const char **JsonMsgTopic);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_GetDisabledTopic
**
** Return a pointer to the table topic entry identified by 'TopicPlugin'.
** 
** Notes:
**   1. A special case arose for getting a disabled topic pointer. Rather than
**      add a parameter MQTT_TOPIC_TBL_GetTopic() in whcih this one case would
**      be the exception, it seemed cleaner to add another function that has
**      some duplicate functionality.
**
*/
const MQTT_TOPIC_TBL_Topic_t *MQTT_TOPIC_TBL_GetDisabledTopic(enum MQTT_GW_TopicPlugin TopicPlugin);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_GetTopic
**
** Return a pointer to the table entry identified by 'TopicPlugin'.
** 
** Notes:
**   1. TopicPlugin must be less than MQTT_GW_TopicPlugin_Enum_t_MAX
**
*/
const MQTT_TOPIC_TBL_Topic_t *MQTT_TOPIC_TBL_GetTopic(enum MQTT_GW_TopicPlugin TopicPlugin);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_GetJsonToCfe
**
** Return a pointer to the JsonToCfe conversion function for 'TopicPlugin'.
** 
** Notes:
**   1. TopicPlugin must be less than MQTT_GW_TopicPlugin_Enum_t_MAX
**
*/
MQTT_TOPIC_TBL_JsonToCfe_t MQTT_TOPIC_TBL_GetJsonToCfe(enum MQTT_GW_TopicPlugin TopicPlugin);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_LoadCmd
**
** Command to copy the table data from a JSON file to memory.
**
** Notes:
**  1. Function signature must match TBLMGR_LoadTblFuncPtr_t.
**  2. Can assume valid table file name because this is a callback from 
**     the app framework table manager that verified the filename.
**
*/
bool MQTT_TOPIC_TBL_LoadCmd(APP_C_FW_TblLoadOptions_Enum_t LoadType, const char *Filename);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_MsgIdToTopicPlugin
**
** Return a topic table index for a message ID
** 
** Notes:
**   1. MQTT_GW_TopicPlugin_UNDEF is returned if the message ID isn't found.
**
*/
uint8 MQTT_TOPIC_TBL_MsgIdToTopicPlugin(CFE_SB_MsgId_t MsgId);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_ResetStatus
**
** Reset counters and status flags to a known reset state.  The behavior of
** the table manager should not be impacted. The intent is to clear counters
** and flags to a known default state for telemetry.
**
*/
void MQTT_TOPIC_TBL_ResetStatus(void);


/******************************************************************************
** Function: MQTT_TOPIC_TBL_RunSbMsgTest
**
** Execute a topic's SB message test.
** 
** Notes:
**   1. Index must be less than MQTT_GW_TopicPlugin_Enum_t_MAX
**
*/
void MQTT_TOPIC_TBL_RunSbMsgTest(enum MQTT_GW_TopicPlugin TopicPlugin, bool Init, int16 Param);


/******************************************************************************
** Function: ValidTopicPlugin
**
** In addition to being in range, valid means that the TopicPlugin has been
** defined.
*/
bool MQTT_TOPIC_TBL_ValidTopicPlugin(enum MQTT_GW_TopicPlugin TopicPlugin);


#endif /* _mqtt_topic_tbl_ */
