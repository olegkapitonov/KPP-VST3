/*
 * Copyright (C) 2018-2020 Oleg Kapitonov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

//-----------------------------------------------------------------------------
// LICENSE
// (c) 2016, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "vstgui/lib/iviewlistener.h"
#include "vstgui/uidescription/icontroller.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

  template <typename ControllerType>
  class PlugUIMessageController : public VSTGUI::IController, public VSTGUI::ViewListenerAdapter
  {
  public:

    using VST3Editor = VSTGUI::VST3Editor;

    enum Tags
    {
      kButtonTag = 1000
    };

    PlugUIMessageController (ControllerType* plugController) : plugController (plugController)
    {
    }
    ~PlugUIMessageController () override
    {
      if (textButton)
        viewWillDelete (textButton);
      plugController->removeUIMessageController (this);
    }

    VST3Editor *mainView;

  private:
    using CControl = VSTGUI::CControl;
    using CView = VSTGUI::CView;
    using CTextButton = VSTGUI::CTextButton;
    using UTF8String = VSTGUI::UTF8String;
    using UIAttributes = VSTGUI::UIAttributes;
    using IUIDescription = VSTGUI::IUIDescription;
    using CNewFileSelector = VSTGUI::CNewFileSelector;
    using UTF8StringPtr = VSTGUI::UTF8StringPtr;
    using CFileExtension = VSTGUI::CFileExtension;

    //--- from IControlListener ----------------------
    void valueChanged (CControl* /*pControl*/) override {}
    void controlBeginEdit (CControl* /*pControl*/) override {}
    void controlEndEdit (CControl* pControl) override
    {
      if (pControl->getTag () == kButtonTag)
      {
        textButton->setTitle("Please wait!");
        VSTGUI::Call::later([this]() { chooseProfileFile(); });
      }
    }

    void chooseProfileFile()
    {
      VSTGUI::SharedPointer<CNewFileSelector> fs(CNewFileSelector::create(mainView->getFrame()));

      fs->setTitle("Load *.tapf Profile file");
      fs->addFileExtension (CFileExtension ("TAPF", "tapf"));
      fs->setDefaultExtension(CFileExtension("TAPF", "tapf"));

      if (fs->runModal()) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
        {
          textButton->setTitle(getBaseName(file));
          plugController->sendTextMessage (file);
          plugController->profilePath = file;
        }
      }
    }
    //--- from IControlListener ----------------------
    //--- is called when a view is created -----
    CView* verifyView (CView* view, const UIAttributes& /*attributes*/,
                       const IUIDescription* /*description*/) override
    {
      if (CTextButton* tb = dynamic_cast<CTextButton*> (view))
      {
        textButton = tb;
        std::string baseName = getBaseName(plugController->profilePath.c_str());
        if (baseName != "")
        {
          textButton->setTitle(baseName.c_str());
        }

        textButton->registerViewListener (this);
      }
      return view;
    }

    void viewWillDelete (CView* view) override
    {
      if (dynamic_cast<CTextButton*> (view) == textButton)
      {
        textButton->unregisterViewListener (this);
        textButton = nullptr;
      }
    }

    void viewLostFocus (CView* view) override
    {
    }

    std::string getBaseName(std::string path)
    {
      std::string baseName = path.substr(path.find_last_of("/") + 1);
      return baseName.substr(0, baseName.find_last_of("."));
    }

    ControllerType* plugController;
    CTextButton* textButton;
  };

} // Vst
} // Steinberg
