//----------------------------------------------------------------------------------------------------
// ID Code      : Eeprom.c No.0002
// Update Note  :
//
//----------------------------------------------------------------------------------------------------

#define __EEPROM__

#include "Core\Header\Include.h"

//--------------------------------------------------
// Eeprom Version Code
//--------------------------------------------------


#if(_MEMORY_LOCATION == _EEPROM)

#define _VERSION_CODE               0x11


/**
* CEepromStartupCheck
* Eeprom check, if version is changed, load default value; else load all data
* @param <none>
* @return {none}
*
*/
void CEepromWriteVersion(void)
{
    pData[0] = _VERSION_CODE;
    CEepromWrite(_SYSTEM_DATA_ADDRESS + 254, 1, pData);
}

void CEepromStartupCheck(void)
{
    BYTE cnt;

    //This delay is for Eeprom power rising time when the connector plug in (VGA_5V or DVI_5V).
    CTimerDelayXms(20);    //CTimerDelayXms(20); 

    for(cnt=0;cnt<2;cnt++)
    {   
        CEepromRead(_SYSTEM_DATA_ADDRESS + 254, 1, pData);
        if(pData[0] == _VERSION_CODE)
            break;
    }
    if(pData[0] != _VERSION_CODE)
    {
        CEepromLoadDefault(1);
        CEepromWriteVersion();
    }
    else
    {
        CEepromLoadSystemData();
        CEepromLoadOsdUserData();
        CEepromLoadAdcData();
        CEepromLoadBriConData();
        CEepromLoadColorTempData();
        CEepromLoadTvData();    
        CEepromLoadHueSatData();
        CEepromLoadAudioData();

    }
}

//--------------------------------------------------
#if (_VIDEO_TV_SUPPORT)
void CRecallTVData(void)
{
    BYTE ucCount = 0;

#if(_NTSC_SEARCH_TABLE)

    bit bTVMode = _GET_TV_TYPE();
    BYTE ucMaxChanel = ((_GET_TV_TYPE() == _TV_CATV) ? _CATV_MAX_CHANNEL : _AIR_MAX_CHANNEL) - 1;
    WORD iFreq;

    // Reset Max channel number
    _SET_MAX_CHANNEL(ucMaxChanel);
    CEepromSaveTvData();

    stTvInfo.CurChn = (bTVMode == _TV_CATV) ? 1 : 2;
    ucPrevChannel   = stTvInfo.CurChn;
    CEepromSaveTvData();

    for(ucCount = 0; ucCount < ucMaxChanel; ucCount++)
    {
        iFreq = (bTVMode == _TV_CATV) ? tNTSC_CATV[ucCount] : tNTSC_AIR[ucCount];
        _SET_CH_FREQ(iFreq);
        _SET_CH_SKIP(1);
        CSaveTVModeData(ucCount);
    }

#else
 
    // Recall freq/tv type/sound type
    ucTVType = _NORMAL_TV_TYPE;
    _SET_CH_COLOR_TYPE(ucTVType);
    _SET_CH_FREQ((WORD)_MIN_FREQ);
    _SET_CH_SKIP(1);
    for(ucCount = 0; ucCount < (_MAX_CHANNEL_COUNT+1); ucCount++)
    {
        CSaveTVModeData(ucCount);
    } 
#endif
}
#endif

//--------------------------------------------------
// Description  : Load eeprom default
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadDefault(bit bMode)
{
    StructModeUserFIFODataType stFIFOModeTemp;

    if((GET_MODE_SEARCH_TYPE() == _USER_MODE_TYPE) && (stModeInfo.ModeCurr < 16))
    {
        CEepromLoadUserFIFOModeData((stModeInfo.ModeCurr / 4), pData);
        stFIFOModeTemp.ModeNum = pData[(stModeInfo.ModeCurr % 4) * 4];
    }

    CEepromInitialModeData();
    CEepromInitialUserFIFOModeData();
    CEepromInitialCenterModeData();
    if (bMode)
    {
       CEepromLoadTvDataDefault();
    }
    CEepromLoadSystemDataDefault();
    CEepromLoadOsdUserDataDefault();
    if (bMode)
    {
       CEepromLoadAdcDataDefault();
       CEepromLoadYPbPrDataDefault();
       CEepromLoadColorTempDataDefault();
    }
    CEepromLoadBriConDataDefault();
    CEepromLoadTvData();
    CEepromLoadHueSatDataDefault();
    CEepromLoadAudioDataDefault();

    if((GET_MODE_SEARCH_TYPE() == _USER_MODE_TYPE) && (stModeInfo.ModeCurr < 16))
    {
        if(stSystemData.UserFIFOMode >= 15)
            stSystemData.UserFIFOMode = 0;
        else
            stSystemData.UserFIFOMode++;

        stFIFOModeTemp.IHFreq  = stModeInfo.IHFreq;
        stFIFOModeTemp.IVFreq  = stModeInfo.IVFreq;
        CEepromSaveUserFIFOModeData(stFIFOModeTemp);

        stModeUserData.FirstAuto    = 0;
        stModeUserData.HPosition    = tINPUTMODE_PRESET_TABLE[stFIFOModeTemp.ModeNum].IHStartPos;
        stModeUserData.VPosition    = tINPUTMODE_PRESET_TABLE[stFIFOModeTemp.ModeNum].IVStartPos;
        stModeUserData.Clock        = tINPUTMODE_PRESET_TABLE[stFIFOModeTemp.ModeNum].IHTotal;
        stModeUserData.Phase        = 0;
        CEepromSaveModeData(stSystemData.UserFIFOMode);
        stModeInfo.ModeCurr = stSystemData.UserFIFOMode;
    }
    else if((GET_MODE_SEARCH_TYPE() == _PRESET_MODE_TYPE) && (stModeInfo.ModeCurr < _MAX_PRESET_MODE))
    {
        CEepromLoadModeData(stModeInfo.ModeCurr);
        CEepromLoadCenterModeData(stModeInfo.ModeCurr);
    }

#if(_VIDEO_TV_SUPPORT)
    if (bMode)
        CRecallTVData();
#endif
}
    
//--------------------------------------------------
// Description  : Eeprom write function
// Input Value  : usAddr        --> absolute address
//                usLength      --> Numbers of data we want to write
//                pWriteArray   --> Writing data array
// Output Value : Return _SUCCESS if succeed
//--------------------------------------------------
bit CEepromWrite(WORD usAddr, WORD usLength, BYTE *pWriteArray)
{
    return CI2cWrite(_EEPROM_ADDRESS + (BYTE)((usAddr & 0xff00) >> 7), (BYTE)(usAddr & 0x00ff), usLength, pWriteArray);
}

/**
* CEepromRead
* Read some byte from Eeprom
* @param <usAddr> {The address in Eeprom}
* @param <usLength> {The number of byte to read}
* @param <pReadArray> {The array to store data}
* @return {Return _SUCCESS if succeed}
*
*/
bit CEepromRead(WORD usAddr, WORD usLength, BYTE *pReadArray)
{
    return CI2cRead(_EEPROM_ADDRESS + (BYTE)((usAddr & 0xff00) >> 7), (BYTE)(usAddr & 0x00ff), usLength, pReadArray);

}


//--------------------------------------------------
// Description  : Save mode setting data, such as FirstAuto,
//                HPosition, VPosition, Clock and Phase.
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveModeData(BYTE ucMode)
{
    if(GET_MODE_SEARCH_TYPE() == _PRESET_MODE_TYPE)
    {
        CEepromWrite(_MODE_DATA_0_ADDRESS + (WORD)ucMode * 8, sizeof(StructModeUserDataType), &stModeUserData.FirstAuto);
    }
    else if(GET_MODE_SEARCH_TYPE() == _USER_MODE_TYPE)
    {
        CEepromWrite(_USER_FIFO_MODE_DATA_1_ADDRESS + (WORD)ucMode * 8, sizeof(StructModeUserDataType), &stModeUserData.FirstAuto);
    }
}

//--------------------------------------------------
// Description  : Load mode setting data, such as FirstAuto,
//                HPosition, VPosition, Clock and Phase.
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadModeData(BYTE ucMode)
{
    if(GET_MODE_SEARCH_TYPE() == _PRESET_MODE_TYPE)
    {
        if(CEepromRead(_MODE_DATA_0_ADDRESS + (WORD)ucMode * 8, sizeof(StructModeUserDataType), &stModeUserData.FirstAuto) == _FAIL)
        {
            stModeUserData.FirstAuto    = 0;
            stModeUserData.HPosition    = stModeInfo.IHStartPos;
            stModeUserData.VPosition    = stModeInfo.IVStartPos;
            stModeUserData.Clock        = stModeInfo.IHTotal;
            stModeUserData.Phase        = 0;
        }
    }
    else if(GET_MODE_SEARCH_TYPE() == _USER_MODE_TYPE)
    {
        if(CEepromRead(_USER_FIFO_MODE_DATA_1_ADDRESS + (WORD)ucMode * 8, sizeof(StructModeUserDataType), &stModeUserData.FirstAuto) == _FAIL)
        {
            stModeUserData.FirstAuto    = 0;
            stModeUserData.HPosition    = stModeInfo.IHStartPos;
            stModeUserData.VPosition    = stModeInfo.IVStartPos;
            stModeUserData.Clock        = stModeInfo.IHTotal;
            stModeUserData.Phase        = 0;
        }
    }
}

//--------------------------------------------------
// Description  : Initial mode setting data with defaults
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromInitialModeData(void)
{
    BYTE modecnt, modetype;

    modetype = GET_MODE_SEARCH_TYPE();
    SET_MODE_SEARCH_TYPE(_PRESET_MODE_TYPE);

    for(modecnt=0;modecnt<_MAX_PRESET_MODE;modecnt++)
    {
        stModeUserData.FirstAuto    = 0;
        stModeUserData.HPosition    = tINPUTMODE_PRESET_TABLE[modecnt].IHStartPos;
        stModeUserData.VPosition    = tINPUTMODE_PRESET_TABLE[modecnt].IVStartPos;
        stModeUserData.Clock        = tINPUTMODE_PRESET_TABLE[modecnt].IHTotal;
        stModeUserData.Phase        = 0;

        CEepromSaveModeData(modecnt);
    }

    for(modecnt=0;modecnt<8;modecnt++)
    {
        CEepromWrite(_USER_FIFO_MODE_DATA_1_ADDRESS + (modecnt * 16), 16, tEEPROM_USER_FIFO_MODE_DEFAULT);
    }

    SET_MODE_SEARCH_TYPE(modetype);
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  : Save user FIFO mode data, such as ModeNum,
//                IHFreq, IVFreq.
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveUserFIFOModeData(StructModeUserFIFODataType stUserFIFOMode)
{
    stUserFIFOMode.IHFreq = ((stUserFIFOMode.IVFreq & 0x0f00) << 4) | (stUserFIFOMode.IHFreq & 0x0fff);
    stUserFIFOMode.IVFreq = ((stUserFIFOMode.IVFreq & 0x00ff) << 8);
    CEepromWrite(_USER_FIFO_MODE_DATA_0_ADDRESS + (stSystemData.UserFIFOMode * 4), 4, &stUserFIFOMode.ModeNum);
}

//--------------------------------------------------
// Description  : Load 4 sets of user FIFO mode data.
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadUserFIFOModeData(BYTE ucNum, BYTE *pArray)
{
    CEepromRead(_USER_FIFO_MODE_DATA_0_ADDRESS + (ucNum * 16), 16, pArray);
}

//--------------------------------------------------
// Description  : Initial user FIFO mode data with defaults
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromInitialUserFIFOModeData(void)
{
    BYTE modecnt;

    for(modecnt=0;modecnt<4;modecnt++)
    {
        CEepromWrite(_USER_FIFO_MODE_DATA_0_ADDRESS + (modecnt * 16), 16, tEEPROM_USER_FIFO_MODE_DEFAULT);
    }
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  :
// Input Value  :
// Output Value : None
//--------------------------------------------------
void CEepromSaveCenterModeData(BYTE ucMode)
{
    if(GET_MODE_SEARCH_TYPE() == _PRESET_MODE_TYPE)
    	CEepromWrite(_MODE_CENTER_DATA_ADDRESS + (WORD)ucMode * 8, sizeof(StructModeUserCenterDataType), (BYTE *)&stModeUserCenterData.CenterHPos);
    else if(GET_MODE_SEARCH_TYPE() == _USER_MODE_TYPE)   
        CEepromWrite(_MODE_CENTER_DATA_ADDRESS + (WORD)_MAX_PRESET_MODE * 8 + (WORD)ucMode * 8, sizeof(StructModeUserCenterDataType), (BYTE *)&stModeUserCenterData.CenterHPos);
}

//--------------------------------------------------
// Description  :
// Input Value  :
// Output Value : None
//--------------------------------------------------
void CEepromLoadCenterModeData(BYTE ucMode)
{
    if(GET_MODE_SEARCH_TYPE() == _PRESET_MODE_TYPE)
    {
        if(CEepromRead(_MODE_CENTER_DATA_ADDRESS + (WORD)ucMode * 8, sizeof(StructModeUserCenterDataType), (BYTE *)&stModeUserCenterData.CenterHPos) == _FAIL)
        {
            stModeUserCenterData.CenterHPos     = stModeInfo.IHStartPos;
            stModeUserCenterData.CenterVPos     = stModeInfo.IVStartPos;
            stModeUserCenterData.CenterClock    = stModeInfo.IHTotal;
        }
    }
    else if(GET_MODE_SEARCH_TYPE() == _USER_MODE_TYPE)
    {
        if(CEepromRead(_MODE_CENTER_DATA_ADDRESS + (WORD)_MAX_PRESET_MODE * 8 + (WORD)ucMode * 8, sizeof(StructModeUserCenterDataType), (BYTE *)&stModeUserCenterData.CenterHPos) == _FAIL)
        {
            stModeUserCenterData.CenterHPos     = stModeInfo.IHStartPos;
            stModeUserCenterData.CenterVPos     = stModeInfo.IVStartPos;
            stModeUserCenterData.CenterClock    = stModeInfo.IHTotal;
        }
    }
}

//--------------------------------------------------
// Description  : Initial mode setting data with defaults
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromInitialCenterModeData(void)
{
    BYTE modecnt, modetype;

    modetype = GET_MODE_SEARCH_TYPE();
    SET_MODE_SEARCH_TYPE(_PRESET_MODE_TYPE);

    for(modecnt=0;modecnt<_MAX_PRESET_MODE;modecnt++)
    {
        stModeUserCenterData.CenterHPos     = tINPUTMODE_PRESET_TABLE[modecnt].IHStartPos;
        stModeUserCenterData.CenterVPos     = tINPUTMODE_PRESET_TABLE[modecnt].IVStartPos;
        stModeUserCenterData.CenterClock    = tINPUTMODE_PRESET_TABLE[modecnt].IHTotal;
        CEepromSaveCenterModeData(modecnt);
    }

    for(modecnt=0;modecnt<8;modecnt++)
    {
        CEepromWrite(_MODE_CENTER_DATA_ADDRESS + (WORD)_MAX_PRESET_MODE * 8 + ((WORD)modecnt * 16), 16, tEEPROM_USER_FIFO_MODE_DEFAULT);
    }

    SET_MODE_SEARCH_TYPE(modetype);
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  : Save system data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveSystemData(void)
{
    CEepromWrite(_SYSTEM_DATA_ADDRESS, sizeof(StructSystemDataType), &stSystemData.MonitorFlag);
    //CEepromWrite(_ICM_COLOR_ADDRESS, sizeof(StructICMColor), &stICMColor.ICM_COLOR[0]);
}

/**
* CEepromLoadSystemData
* Load system data
* @param <none>
* @return {none}
*
*/
void CEepromLoadSystemData(void)
{
    CEepromRead(_SYSTEM_DATA_ADDRESS, sizeof(StructSystemDataType), &stSystemData.MonitorFlag);
    //CEepromRead(_ICM_COLOR_ADDRESS, sizeof(StructICMColor), &stICMColor.ICM_COLOR[0]);
}

//--------------------------------------------------
// Description  : Load default system data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadSystemDataDefault(void)
{
    stSystemData = tEEPROM_SYSTEM_DATA;
    //stICMColor = tEEPROM_ICM_COLOR;
    CEepromWrite(_SYSTEM_DATA_ADDRESS, sizeof(StructSystemDataType), &tEEPROM_SYSTEM_DATA.MonitorFlag);
    //CEepromWrite(_ICM_COLOR_ADDRESS, sizeof(StructICMColor), &tEEPROM_ICM_COLOR.ICM_COLOR[0]);
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  : Save OSD data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveOsdUserData(void)
{
    CEepromWrite(_OSDUSER_DATA_ADDRESS, sizeof(StructOsdUserDataType), &stOsdUserData.OsdHPos);
}

/**
* CEepromLoadOsdUserData
* Load OSD data
* @param <none>
* @return {none}
*
*/
void CEepromLoadOsdUserData(void)
{
    CEepromRead(_OSDUSER_DATA_ADDRESS, sizeof(StructOsdUserDataType), &stOsdUserData.OsdHPos);
}

//--------------------------------------------------
// Description  : Load default OSD data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadOsdUserDataDefault(void)
{
    CEepromWrite(_OSDUSER_DATA_ADDRESS, sizeof(StructOsdUserDataType), &tEEPROM_OSDUSER_DATA.OsdHPos);
    stOsdUserData = tEEPROM_OSDUSER_DATA;
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  : Save ADC data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveAdcData(void)
{
    if(_GET_INPUT_SOURCE() == _SOURCE_VGA)
    	CEepromWrite(_ADC_DATA_ADDRESS, sizeof(StructAdcDataType), &stAdcData.AdcGain[_RED]);
    else
    	CEepromWrite(_YPbPr_DATA_ADDRESS, sizeof(StructYPbPrDataType), &stYPbPrData.YPbPrGain[_RED]);
}

/**
* CEepromLoadAdcData
* Load ADC data
* @param <none>
* @return {none}
*
*/
void CEepromLoadAdcData(void)
{
    CEepromRead(_ADC_DATA_ADDRESS, sizeof(StructAdcDataType), &stAdcData.AdcGain[_RED]);
    CEepromRead(_YPbPr_DATA_ADDRESS, sizeof(StructYPbPrDataType), &stYPbPrData.YPbPrGain[_RED]);
}

//--------------------------------------------------
// Description  : Load default ADC data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadAdcDataDefault(void)
{
    CEepromWrite(_ADC_DATA_ADDRESS, sizeof(StructAdcDataType), &tEEPROM_ADC_DATA.AdcGain[_RED]);
    stAdcData = tEEPROM_ADC_DATA;
}

//--------------------------------------------------
// Description  : Load default ADC data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadYPbPrDataDefault(void)
{
    CEepromWrite(_YPbPr_DATA_ADDRESS, sizeof(StructYPbPrDataType), &tEEPROM_YPbPr_DATA.YPbPrGain[_RED]);
    stYPbPrData = tEEPROM_YPbPr_DATA;
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  : Save Brightness/Contrast data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveBriConData(void)
{
    CEepromWrite(_COLOR_PROC0_ADDRESS + (CGetInputSourceNum(_GET_INPUT_SOURCE()) * 2), 2, &stConBriData.Brightness);
}

/**
* CEepromLoadBriConData
* Load Brightness and Contrast data
* @param <none>
* @return {none}
*
*/
void CEepromLoadBriConData(void)
{
     CEepromRead(_COLOR_PROC0_ADDRESS + (CGetInputSourceNum(_GET_INPUT_SOURCE()) * 2), 2, &stConBriData.Brightness);
}

//--------------------------------------------------
// Description  : Load default Brightness/Contrast data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadBriConDataDefault(void)
{
    CEepromWrite(_COLOR_PROC0_ADDRESS, 2 * 4, &tEEPROM_COLOR_PROC0_DATA[0].Brightness);
 //   CEepromWrite(_COLOR_PROC0_ADDRESS, 2 * 7, &tEEPROM_COLOR_PROC0_DATA[0].Brightness);
    stConBriData = tEEPROM_COLOR_PROC0_DATA[0];
}

//----------------------------------------------------------------------------------------------------


// Description  : Save Color Temperature data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveColorTempData(void)
{
    CEepromWrite(_COLORTEMP_DATA_ADDRESS + 15 * CGetInputSourceNum(_GET_INPUT_SOURCE()) + 3 * GET_COLOR_TEMP_TYPE(), 3, &stColorTempData.ColorTemp[_RED]);
}

/**
* CEepromLoadColorTempData
* Load Color Temperature data
* @param <none>
* @return {none}
*
*/
void CEepromLoadColorTempData(void)
{
     CEepromRead(_COLORTEMP_DATA_ADDRESS + 15 * CGetInputSourceNum(_GET_INPUT_SOURCE()) + 3 * GET_COLOR_TEMP_TYPE(), 3, &stColorTempData.ColorTemp[_RED]);
}

//--------------------------------------------------
// Description  : Load default Color Temperature data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadColorTempDataDefault(void)
{
    CEepromWrite(_COLORTEMP_DATA_ADDRESS, 15 * 4, &tEEPROM_COLORTEMP_DATA[0].ColorTemp[0]);
    CEepromLoadColorTempData(); 
}

//----------------------------------------------------------------------------------------------------

//--------------------------------------------------
// Description  : Save tvInfo data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveTvData(void)
{
    CEepromWrite(_TV_DATA_ADDRESS, sizeof(StructTvDataType), &stTvInfo.CurChn);
}

/**
* CEepromLoadTvData
* Load TV information data
* @param <none>
* @return {none}
*
*/
void CEepromLoadTvData(void)
{
    CEepromRead(_TV_DATA_ADDRESS, sizeof(StructTvDataType), &stTvInfo.CurChn);
#if(_NTSC_SEARCH_TABLE)

	if (_GET_TV_TYPE() == _TV_CATV)	_SET_MAX_CHANNEL(_CATV_MAX_CHANNEL - 1);
	else							_SET_MAX_CHANNEL(_AIR_MAX_CHANNEL - 1);

#else

#if(_CHANGE_CHANNEL_MTD == _CHANGE_CHANNEL_M2)
    _SET_MAX_CHANNEL(_MAX_CHANNEL_COUNT);
#else

    if ((_MAX_CHANNEL_COUNT < _GET_MAX_CHANNEL()) || (_GET_MAX_CHANNEL() == 0))
    {
        // Save search channel number
        _SET_MAX_CHANNEL(1);
        CEepromSaveTvData();
    }

#endif // #if(_CHANGE_CHANNEL_MTD == _CHANGE_CHANNEL_M2)

#endif // #if(_NTSC_SEARCH_TABLE)
}
        
//--------------------------------------------------
// Description  : Load default system data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadTvDataDefault(void)
{
    stTvInfo = tEEPROM_TV_DATA;
    CEepromSaveTvData();
}
 
//--------------------------------------------------
// Description  : Save tAudioData data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromSaveAudioData(void)
{
    CEepromWrite(_AUDIO_DATA_ADDRESS, sizeof(StructAudioDataType), &stAudioData.Mode);
}

/**
* CEepromLoadAudioData
* Load Audio data
* @param <none>
* @return {none}
*
*/
void CEepromLoadAudioData(void)
{
    CEepromRead(_AUDIO_DATA_ADDRESS, sizeof(StructAudioDataType), &stAudioData.Mode);
}

//--------------------------------------------------
// Description  : Load default system data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadAudioDataDefault(void)
{
    stAudioData = tEEPROM_AUDIO_DATA;
    CEepromSaveAudioData();
}

//lzg 20060112 add picture mode data eeprom for sva
//below you can add the customer extra load/save variable function
//////////////////////////////////////////////////////////////////

void CEepromSaveHueSatData(void)
{
    CEepromWrite(_HueSaturation_DATA_ADDRESS + (CGetInputSourceNum(_GET_INPUT_SOURCE()) * 2), 2, &stHueSatData.Hue);    
}

void CEepromLoadHueSatData(void)
{
    CEepromRead(_HueSaturation_DATA_ADDRESS + (CGetInputSourceNum(_GET_INPUT_SOURCE()) * 2), 2, &stHueSatData.Hue); 
}

//--------------------------------------------------
// Description  : Load default HueSat data
// Input Value  : None
// Output Value : None
//--------------------------------------------------
void CEepromLoadHueSatDataDefault(void)
{
    CEepromWrite(_HueSaturation_DATA_ADDRESS, 2 * 4, &tEEPROM_HUE_SAT_DATA[0].Hue);
    //CEepromLoadHueSatData();
    stHueSatData = tEEPROM_HUE_SAT_DATA[0];
}


#if(_VIDEO_TV_SUPPORT)
//--------------------------------------------------
//==========================================================================
//                       CGetTVEepromAddr
//==========================================================================
BYTE CGetTVEepromAddr(BYTE ucCurrentChannel)
{
    BYTE ucEeprom_Addr;

    if (ucCurrentChannel > 0)
       ucCurrentChannel -= 1;

#if(_FM_DEVICE)
    if (bFM)
       ucEeprom_Addr = _TV_FREQ_EEPROM_ADDR0;
    else
#endif
    ucEeprom_Addr = (_TV_FREQ_EP0_TV_NUMBER < ucCurrentChannel) ? _TV_FREQ_EEPROM_ADDR1 : _TV_FREQ_EEPROM_ADDR0;

    return ucEeprom_Addr;
}

//==========================================================================
//                      CGetTVStartAddr
//==========================================================================
BYTE CGetTVStartAddr(BYTE ucCurrentChannel)
{
    BYTE ucStart_Addr;

    if (ucCurrentChannel > 0)
       ucCurrentChannel -= 1;

#if(_FM_DEVICE)
    if (bFM)
        ucStart_Addr = _FM_START_ADDR + (ucCurrentChannel * 2);
    else
#endif
    ucStart_Addr = (_TV_FREQ_EP0_TV_NUMBER < ucCurrentChannel) ? ((ucCurrentChannel - (_TV_FREQ_EP0_TV_NUMBER+1)) * 2)
                                                : (_TV_FREQ_EP_START_ADDR0 + (ucCurrentChannel * 2));

    return ucStart_Addr;
}


//==========================================================================
//                          CSaveChannelData
//==========================================================================
void CSaveTVModeData(BYTE ucChannel)
{
    pData[0] = stTVChannelInfo.Freq & 0xff;
    pData[1] = (stTVChannelInfo.Freq >> 8) & 0xff;
    CI2cWrite(CGetTVEepromAddr(ucChannel), CGetTVStartAddr(ucChannel), LENGTH(2), pData);  // eric 0223
    CTimerDelayXms(4);

#if(_FM_DEVICE)
    if (bFM)
       return;
#endif

    if (ucChannel > 0)
       ucChannel -= 1;

    // Save sound and tv system
    pData[0] = stTVChannelInfo.TYPE;
    CI2cWrite(_TV_SYSTEM_EEPROM_ADDR, _TV_SYSTEM_START_ADDR+ucChannel, LENGTH(1), pData);
    CTimerDelayXms(4);
}

//==========================================================================
//                          CSaveChannelData
//==========================================================================
void CLoadTVModeData(BYTE ucChannel)
{
    // Freq
    CI2cRead(CGetTVEepromAddr(ucChannel), CGetTVStartAddr(ucChannel), LENGTH(2), pData);  // eric 0223

    if ((pData[1] & 0x80))
        _SET_CH_SKIP(1);
    else
        _SET_CH_SKIP(0);
    pData[1] &= 0x7f;
    _SET_CH_FREQ((256*pData[1]) + pData[0]);

    if (_MAX_FREQ < _GET_CH_FREQ())
        _SET_CH_FREQ((WORD)_MAX_FREQ);
    else if(_MIN_FREQ > _GET_CH_FREQ())
        _SET_CH_FREQ((WORD)_MIN_FREQ);

    if (ucChannel > 0)
        ucChannel -= 1;

    // Color and sound type
    CI2cRead(_TV_SYSTEM_EEPROM_ADDR, _TV_SYSTEM_START_ADDR+ucChannel, LENGTH(1), pData);    // Read current channel sound and tv system
    stTVChannelInfo.TYPE = pData[0]; // tv type

    if ((_MAX_TV_TYPE < _GET_CH_COLOR_TYPE()) || (0 == _GET_CH_COLOR_TYPE()))
        _SET_CH_COLOR_TYPE(_NORMAL_TV_TYPE); // Set to normal PAL I
 
    ucTVType =_GET_CH_COLOR_TYPE();
}

#endif // #if(_VIDEO_TV_SUPPORT)

#endif		//#if(_MEMORY_LOCATION == _EEPROM)

//----------------------------------------------------------------------------------------------------
