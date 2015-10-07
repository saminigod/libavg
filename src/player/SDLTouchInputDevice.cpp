//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2014 Ulrich von Zadow
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at www.libavg.de
//

#include "SDLTouchInputDevice.h"
#include "TouchEvent.h"
#include "Player.h"
#include "AVGNode.h"
#include "TouchStatus.h"
#include "TouchEvent.h"
#include "SDLWindow.h"
#include "DisplayEngine.h"

#include "../base/Logger.h"
#include "../base/ObjectCounter.h"
#include "../base/Exception.h"
#include "../base/ConfigMgr.h"

using namespace std;

namespace avg {

#ifdef _WIN32
SDLTouchInputDevice* SDLTouchInputDevice::s_pInstance(0);
#endif

SDLTouchInputDevice::SDLTouchInputDevice(const DivNodePtr& pEventReceiverNode)
    : MultitouchInputDevice(pEventReceiverNode)
{
#ifdef _WIN32
    s_pInstance = this;
    HWND hwnd = Player::get()->getDisplayEngine()->getWindow(0)->getWinHWnd();
    m_OldWndProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)touchWndSubclassProc);
#endif
}

#ifdef _WIN32
#define MOUSEEVENTF_FROMTOUCH 0xFF515700

LRESULT APIENTRY SDLTouchInputDevice::touchWndSubclassProc(HWND hwnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    // This is called before SDL event processing and discards all mouse events that have
    // corresponding touch events.
    SDLTouchInputDevice * pThis = SDLTouchInputDevice::s_pInstance;
    bool bIgnore = false;
    switch(uMsg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            if (GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) {
                // Click was generated by wisptis / Windows Touch
                bIgnore = true;
            }
            break;
        case WM_MOUSEMOVE:
            // Workaround for Win7 bug: There is no way to figure out if a WM_MOUSEMOVE
            // originates from a touch, so we ignore all mousemove events as long as
            // something is touching the screen.
            bIgnore = (pThis->getNumTouches() > 0);
            break;
        default:
            break;
    }
    if (!bIgnore) {
        return CallWindowProc(pThis->m_OldWndProc, hwnd, uMsg, wParam, lParam);
    } else {
        return 0;
    }
}
#endif

void SDLTouchInputDevice::onTouchEvent(SDLWindow* pWindow, const SDL_Event& sdlEvent)
{
    EventPtr pNewEvent;
    AVG_ASSERT(sdlEvent.type == SDL_FINGERDOWN || sdlEvent.type == SDL_FINGERMOTION 
            || sdlEvent.type == SDL_FINGERUP);
    SDL_TouchFingerEvent fingerEvent = sdlEvent.tfinger;
    glm::vec2 normPos(fingerEvent.x, fingerEvent.y);
    IntPoint winSize(Player::get()->getRootNode()->getSize());
#ifdef _WIN32
    IntPoint pos = normPos * glm::vec2(winSize);
#else
    IntPoint pos = normPos;
#endif
    pos.x = min(max(pos.x, 0), winSize.x-1);
    pos.y = min(max(pos.y, 0), winSize.y-1);
    switch (sdlEvent.type) {
        case SDL_FINGERDOWN:
            {
 //               cerr << "down: " << pos << endl;
                TouchEventPtr pEvent (new TouchEvent(getNextContactID(), 
                        Event::CURSOR_DOWN, pos, Event::TOUCH));
                addTouchStatus((int)fingerEvent.fingerId, pEvent);
            }
            break;
        case SDL_FINGERUP:
            {
 //               cerr << "up: " << pos << endl;
                TouchStatusPtr pTouchStatus = getTouchStatus((int)fingerEvent.fingerId);
                CursorEventPtr pOldEvent = pTouchStatus->getLastEvent();
                TouchEventPtr pUpEvent(new TouchEvent(pOldEvent->getCursorID(), 
                        Event::CURSOR_UP, pos, Event::TOUCH));
                pTouchStatus->pushEvent(pUpEvent);
            }
            break;
        case SDL_FINGERMOTION:
            {
 //               cerr << "motion: " << pos << endl;
                TouchEventPtr pEvent(new TouchEvent(0, Event::CURSOR_MOTION, pos,
                        Event::TOUCH));
                TouchStatusPtr pTouchStatus = getTouchStatus((int)fingerEvent.fingerId);
                AVG_ASSERT(pTouchStatus);
                pTouchStatus->pushEvent(pEvent);
            }
            break;
        default:
            AVG_ASSERT(false);
            break;
    }
}

}