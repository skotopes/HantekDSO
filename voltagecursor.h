/*
    Copyright (c) 2010, Thomas Gerner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef VOLTAGECURSOR_H
#define VOLTAGECURSOR_H

#include <cursor.h>
#include "glbox.h"


class VoltageCursor : public Cursor
{
public:
    VoltageCursor(QWidget* parent);
    virtual ~VoltageCursor();
    
    virtual void setPosition(QMouseEvent *e, int width, int height);
    virtual bool isSelected(QMouseEvent *e, int width, int height);
    virtual GLfloat getLower();
    virtual GLfloat getUpper();
    virtual void paintCursor(SansFont& font);
    
    void setChannel(channels ch);
    void setChActive(bool active);
    void setChannelVoltage(double offset, double div);
private:
    double chOffset, chDiv;
    bool chActive;
    channels channel;
};

#endif // VOLTAGECURSOR_H
