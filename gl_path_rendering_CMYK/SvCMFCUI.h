/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ //--------------------------------------------------------------------
#ifdef USESVCUI
#include "ISvcUI.h"
#ifdef EXTERNSVCUI
extern nvSvc::ISvcFactory* g_pFactUI;
extern IWindowHandler *    g_pWinHandler;
extern IWindowConsole*     g_pConsole;
extern IWindowLog*         g_pLog;
extern IProgressBar*       g_pProgress;
extern IWindowFolding*     g_pToggleContainer;
extern void addToggleKeyToMFCUI(char c, bool* target, const char* desc);
extern void shutdownMFCUI();
extern void initMFCUIBase(int x=0, int y=600, int w=400, int h=100);
extern void logMFCUI(int level, const char * txt);
extern void flushMFCUIToggle(int key);
#else
nvSvc::ISvcFactory* g_pFactUI       = NULL;
IWindowHandler *    g_pWinHandler   = NULL;
IWindowConsole*     g_pConsole      = NULL;
IWindowLog*         g_pLog          = NULL;
IProgressBar*       g_pProgress     = NULL;

IWindowFolding*   g_pToggleContainer = NULL;
void addToggleKeyToMFCUI(char c, bool* target, const char* desc)
{
    if(!g_pToggleContainer)
        return;
    g_pToggleContainer->UnFold(0);
    g_pWinHandler->VariableBind(g_pWinHandler->CreateCtrlCheck((LPCSTR)c, desc, g_pToggleContainer), target);
    g_pToggleContainer->UnFold();
}

void shutdownMFCUI()
{
    g_pConsole = NULL;
    g_pLog = NULL;
    if(g_pWinHandler) g_pWinHandler->DestroyAll();
    UISERVICE_UNLOAD(g_pFactUI, g_pWinHandler);
}

//------------------------------------------------------------------------------
// Setup the base layout of the UI
// the rest can be done outside, depending on the sample's needs
void initMFCUIBase(int x=0, int y=600, int w=400, int h=100)
{
    UISERVICE_LOAD(g_pFactUI, g_pWinHandler);
    if(g_pWinHandler)
    {
        // a Log window is a line-by-line logging, with possible icons for message levels
        g_pLog= g_pWinHandler->CreateWindowLog("LOG", "Log");
        g_pLog->SetVisible()->SetLocation(x,y)->SetSize(w-(w*30/100),h);
        (g_pToggleContainer = g_pWinHandler->CreateWindowFolding("TOGGLES", "Toggles", NULL))
            ->SetLocation(x+(w*70/100), y)
            ->SetSize(w*30/100, h)
            ->SetVisible();
        // Console is a window in which you can write and capture characters the user typed...
        //g_pConsole = g_pWinHandler->CreateWindowConsole("CONSOLE", "Console");
        //g_pConsole->SetVisible();//->SetLocation(0,m_winSize[1]+32)->SetSize(m_winSize[0],200);
        // Show and update this control when doing long load/computation... for example
        g_pProgress = g_pWinHandler->CreateWindowProgressBar("PROG", "Loading", NULL);
        g_pProgress->SetVisible(0);
    }
}

void logMFCUI(int level, const char * txt)
{
   if(g_pLog)
        g_pLog->AddMessage(level, txt);
}

extern std::map<char, bool*>    g_toggleMap;
void flushMFCUIToggle(int key)
{
    std::map<char, bool*>::iterator it = g_toggleMap.find(key);
    if(it != g_toggleMap.end())
        g_pWinHandler->VariableFlush(it->second);
}
#endif
#endif
