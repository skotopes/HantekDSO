/***************************************************************************
 *   Copyright (C) 2008 by Oleg Khudyakov                                  *
 *   prcoder@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <qlabel.h>
#include <qslider.h>
#include <qscrollbar.h>
#include <qcheckbox.h>
#include <math.h>
#include "hantekdsowidget.h"
#include "hantekdso.h"
#include "glbox.h"
#include "timecursor.h"
#include "voltagecursor.h"
#include <QSettings>

const double HantekDSOWidget::voltagePerDiv[] = {
	5.0, 2.0, 1.0, 0.5, 0.2, 0.1, 0.05, 0.02, 0.01
};

HantekDSOWidget::HantekDSOWidget(QWidget* parent, const char* name)
        : timeBase(TIME_1ms),
          selectedChannel(SELECT_CH1CH2), ch1Active(1), ch2Active(1),
          ch1Voltage(VOLTAGE_5V), ch2Voltage(VOLTAGE_5V),
          ch1Atten(0), ch2Atten(0),
          ch1Coupling(COUPLING_DC), ch2Coupling(COUPLING_DC),
          ch1Offset(0), ch2Offset(0), triggerOffset(0),
          prevCh1SliderValue(-1), prevCh2SliderValue(-1),
          ch1Filter(0), ch2Filter(0), triggerFilter(0),
          triggerSource(0), triggerSlope(SLOPE_POSITIVE),
          triggerPosition(0x78A60), calData(0)
{
    gLBox1->setAThread(&dsoAThread);
    cursorCh1->setChannel(CHANNEL_CH1);
    cursorCh2->setChannel(CHANNEL_CH2);
    gLBox1->setCursors(timeCursor, cursorCh1, cursorCh2);
    connect(&refreshTimer, SIGNAL(timeout()), this, SLOT(refreshView()));
    readSetting();
    startDSO();
}

HantekDSOWidget::~HantekDSOWidget()
{
    refreshTimer.stop();
    dsoAThread.stop();
    dsoAThread.wait();
}

/*$SPECIALIZATION$*/

void HantekDSOWidget::polish(void)
{
}


/*!
    \fn HantekDSOWidget::startDSO()
 */
int HantekDSOWidget::startDSO()
{
    if (dsoAThread.dsoIO.dsoInit() == 0)
    {
        textInfo->setText(QString("DSO-%1 found").arg(dsoAThread.dsoIO.dsoGetModel(), 0, 16));

        if (dsoAThread.dsoIO.dsoGetChannelLevel(&chLevels) < 0)
        {
            qDebug("Error running GetChannelLevel command");
        }
/*
        for(int i=0; i<2; i++)
            for(int j=0; j<9; j++)
                for(int k=0; k<2; k++)
                    qDebug("%04X", chLevels[i][j][k]);
*/
        dsoAThread.getCalData();

        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel, triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }

        if (dsoAThread.dsoIO.dsoSetFilter(ch1Filter, ch2Filter, triggerFilter) < 0)
        {
            qDebug("Error running SetFilter command");
        }

        if (dsoAThread.dsoIO.dsoSetVoltageAndCoupling(ch1Voltage, ch2Voltage,
            ch1Coupling, ch2Coupling, triggerSource) < 0)
        {
            qDebug("Error running SetVoltageAndCoupling command");
        }
        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel,
            triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }

        sliderCh1_valueChanged(sliderCh1->value());
        sliderCh2_valueChanged(sliderCh2->value());
        sliderChM_valueChanged(sliderChM->value());
        sliderTrigger_valueChanged(sliderTrigger->value());
        setActiveChannel();
        dsoAThread.start();

        refreshTimer.start(25);
    }
    else
    {
        textInfo->setText("DSO NOT found");
    }

    return 0;
}


void HantekDSOWidget::buttonStart_clicked()
{
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        int triggerMode = dsoAThread.getTriggerMode();

        if (dsoAThread.dsoIO.dsoCaptureStart() < 0)
        {
            qDebug("Error running CaptureStart command");
        }

        if (dsoAThread.dsoIO.dsoTriggerEnabled() < 0)
        {
            qDebug("Error running TriggerEnabled command");
        }

        if (triggerMode == TRIGGER_MODE_AUTO)
        {
            if (dsoAThread.dsoIO.dsoForceTrigger() < 0)
            {
                qDebug("Error running ForceTrigger command");
            }
        }
    }
    qDebug("START");
}

void HantekDSOWidget::buttonStop_clicked()
{
    qDebug("STOP");
}


/*!
    \fn HantekDSOWidget::setActiveChannel()
 */
void HantekDSOWidget::setActiveChannel()
{
    int prevSelectedChannel = selectedChannel;

    if (ch1Active & ch2Active)
    {
        selectedChannel = SELECT_CH1CH2;
    }
    else
    {
        if (ch1Active)
        {
            selectedChannel = SELECT_CH1;
        }
        if (ch2Active)
        {
            selectedChannel = SELECT_CH2;
        }
    }

    if (selectedChannel != prevSelectedChannel && dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel,
            triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }
    }

    gLBox1->setActiveChannels(ch1Active, ch2Active);
    cursorCh1->setChActive(ch1Active != 0);
    cursorCh2->setChActive(ch2Active != 0);
}

void HantekDSOWidget::comboVoltage1_activated(int voltage)
{
    ch1Voltage = voltage;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetVoltageAndCoupling(ch1Voltage, ch2Voltage,
            ch1Coupling, ch2Coupling,  triggerSource) < 0)
        {
            qDebug("Error running SetVoltageAndCoupling command");
        }
    }
    sliderCh1_valueChanged(sliderCh1->value());
}

void HantekDSOWidget::comboCoupling1_activated(int coupling)
{
    if (coupling != COUPLING_OFF)
    {
        ch1Active = CHANNEL_ACTIVE;
        ch1Coupling = coupling;
        if (dsoAThread.dsoIO.dsoIsFound())
        {
            if (dsoAThread.dsoIO.dsoSetVoltageAndCoupling(ch1Voltage, ch2Voltage,
                ch1Coupling, ch2Coupling,  triggerSource) < 0)
            {
                qDebug("Error running SetVoltageAndCoupling command");
            }
        }
    }
    else
    {
        ch1Active = CHANNEL_OFF;
    }

    setActiveChannel();
}

void HantekDSOWidget::comboAtten1_activated(int atten)
{
    qDebug("Attenuator1=%i", atten);
    ch1Atten = pow(10, atten);
    sliderCh1_valueChanged(sliderCh1->value());
}

void HantekDSOWidget::comboVoltage2_activated(int voltage)
{
    ch2Voltage = voltage;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetVoltageAndCoupling(ch1Voltage, ch2Voltage,
            ch1Coupling, ch2Coupling,  triggerSource) < 0)
        {
            qDebug("Error running SetVoltageAndCoupling command");
        }
    }
    sliderCh2_valueChanged(sliderCh2->value());
}

void HantekDSOWidget::comboCoupling2_activated(int coupling)
{
    if (coupling != COUPLING_OFF)
    {
        ch2Active = CHANNEL_ACTIVE;
        ch2Coupling = coupling;
        if (dsoAThread.dsoIO.dsoIsFound())
        {
            if (dsoAThread.dsoIO.dsoSetVoltageAndCoupling(ch1Voltage, ch2Voltage,
                ch1Coupling, ch2Coupling, triggerSource) < 0)
            {
                qDebug("Error running SetVoltageAndCoupling command");
            }
        }
    }
    else
    {
        ch2Active = CHANNEL_OFF;
    }

    setActiveChannel();
}

void HantekDSOWidget::comboAtten2_activated(int atten)
{
    qDebug("Attenuator2=%i", atten);
    ch2Atten = pow(10, atten);
    sliderCh2_valueChanged(sliderCh2->value());
}


void HantekDSOWidget::comboTimeBase_activated(int timeBase)
{
    this->timeBase = timeBase;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel,
            triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }
    }
    
    double base = 1 << (timeBase % 3);
    base *= pow(10, timeBase / 3);
    base *= 1e-5;
    timeCursor->setTimebase(base);
}

void HantekDSOWidget::sliderCh1_valueChanged(int value)
{
    unsigned offsetStart = chLevels[SELECT_CH1][VOLTAGE_10mV-ch1Voltage][OFFSET_START];
    unsigned offsetEnd = chLevels[SELECT_CH1][VOLTAGE_10mV-ch1Voltage][OFFSET_END];
    unsigned offsetRange = offsetEnd - offsetStart;

    unsigned sliderStart = sliderCh1->minimum();
    unsigned sliderEnd = sliderCh1->maximum();
    unsigned sliderRange = sliderEnd - sliderStart;

    ch1Offset = (sliderEnd - value)*offsetRange/sliderRange + offsetStart;
    ch1Offset >>= 8;
    
    if (triggerSource == 1 && prevCh1SliderValue > -1)
    {
        int sliderMoved = prevCh1SliderValue - value;
        
        unsigned triggerSliderStart = sliderTrigger->minimum();
        unsigned triggerSliderEnd = sliderTrigger->maximum();
        unsigned triggerSliderRange = triggerSliderEnd - triggerSliderStart;
        
        int triggerSliderMove = sliderMoved * (int) triggerSliderRange / (int) sliderRange;
        int triggerSliderValue = sliderTrigger->value();
        sliderTrigger->setValue(triggerSliderValue - triggerSliderMove);
    }
    else if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetOffset(ch1Offset, ch2Offset, triggerOffset) < 0)
        {
            qDebug("Error running SetOffset command");
        }
    }
    prevCh1SliderValue = value;
    
    double ch1Div = voltagePerDiv[ch1Voltage] * ch1Atten;
    double offset = DIVS_VOLTAGE * (double) value / (double) sliderRange - DIVS_VOLTAGE/2;
    cursorCh1->setChannelVoltage(offset * ch1Div, ch1Div);
}

void HantekDSOWidget::sliderCh2_valueChanged(int value)
{
    unsigned offsetStart = chLevels[SELECT_CH2][VOLTAGE_10mV-ch2Voltage][OFFSET_START];
    unsigned offsetEnd = chLevels[SELECT_CH2][VOLTAGE_10mV-ch2Voltage][OFFSET_END];
    unsigned offsetRange = offsetEnd - offsetStart;

    unsigned sliderStart = sliderCh2->minimum();
    unsigned sliderEnd = sliderCh2->maximum();
    unsigned sliderRange = sliderEnd - sliderStart;

    ch2Offset = (sliderEnd - value)*offsetRange/sliderRange + offsetStart;
    ch2Offset >>= 8;

    
    if (triggerSource == 0 && prevCh2SliderValue > -1)
    {
        int sliderMoved = prevCh2SliderValue - value;
        
        unsigned triggerSliderStart = sliderTrigger->minimum();
        unsigned triggerSliderEnd = sliderTrigger->maximum();
        unsigned triggerSliderRange = triggerSliderEnd - triggerSliderStart;
        
        int triggerSliderMove = sliderMoved * (int) triggerSliderRange / (int) sliderRange;
        int triggerSliderValue = sliderTrigger->value();
        sliderTrigger->setValue(triggerSliderValue - triggerSliderMove);
    }
    else if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetOffset(ch1Offset, ch2Offset, triggerOffset) < 0)
        {
            qDebug("Error running SetOffset command");
        }
    }
    prevCh2SliderValue = value;
    
    double ch2Div = voltagePerDiv[ch2Voltage] * ch2Atten;
    double offset = DIVS_VOLTAGE * (double) value / (double) sliderRange - DIVS_VOLTAGE/2;
    cursorCh2->setChannelVoltage(offset * ch2Div, ch2Div);
}

void HantekDSOWidget::sliderChM_valueChanged(int value)
{
    unsigned offsetStart = 0;
    unsigned offsetEnd = VOLTAGE_SCALE;
    unsigned offsetRange = offsetEnd - offsetStart;

    unsigned sliderStart = sliderChM->minimum();
    unsigned sliderEnd = sliderChM->maximum();
    unsigned sliderRange = sliderEnd - sliderStart;

    int chMOffset = (sliderEnd - value)*offsetRange/sliderRange + offsetStart;
    gLBox1->setChMOffset(chMOffset);
}

void HantekDSOWidget::sliderTrigger_valueChanged(int value)
{
    unsigned offsetStart = 0x00;
    unsigned offsetEnd = 0xFE;
    unsigned offsetRange = offsetEnd - offsetStart;

    unsigned sliderStart = sliderTrigger->minimum();
    unsigned sliderEnd = sliderTrigger->maximum();
    unsigned sliderRange = sliderEnd - sliderStart;

    triggerOffset = (sliderEnd - value)*offsetRange/sliderRange + offsetStart;

    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetOffset(ch1Offset, ch2Offset, triggerOffset) < 0)
        {
            qDebug("Error running SetOffset command");
        }
    }
}


/*!
    \fn HantekDSOWidget::refreshView()
 */
void HantekDSOWidget::refreshView()
{
    gLBox1->update();
}

void HantekDSOWidget::comboDisplayInterpolation_activated(int state)
{
    gLBox1->setInterpolation(state);
}

void HantekDSOWidget::checkDigitalPhosphor_stateChanged(int state)
{
    gLBox1->setDigitalPhosphor(state);
}

void HantekDSOWidget::checkDSound_stateChanged(int state)
{
    qDebug("DigitalSound=%i", state);
}

void HantekDSOWidget::comboViewMode_activated(int mode)
{
    gLBox1->setViewMode(mode);
}

void HantekDSOWidget::comboMathType_activated(int type)
{
    gLBox1->setMathType(type);
}

void HantekDSOWidget::sliderTriggerPos_valueChanged(int value)
{
    triggerPosition = 0x77660 + value;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel,
            triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }
    }
}

void HantekDSOWidget::comboTriggerSource_activated(int source)
{
    triggerSource = source;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel,
            triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }
    }
}

void HantekDSOWidget::comboTriggerSlope_activated(int slope)
{
    triggerSlope = slope;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetTriggerAndSampleRate(timeBase, selectedChannel,
            triggerSource, triggerSlope, triggerPosition, dsoAThread.bufferSize) < 0)
        {
            qDebug("Error running SetTriggerAndSampleRate command");
        }
    }
}

void HantekDSOWidget::comboTriggerMode_activated(int mode)
{
    dsoAThread.setTriggerMode(mode);
}

void HantekDSOWidget::timeDiv_activated(int div)
{
    qDebug("div=%i", div);
    double timeDiv = 1 << (div % 3);
    timeDiv *= pow(10, div / 3);
    qDebug("TimeDiv=%f", timeDiv);
    gLBox1->setTimeDiv(timeDiv);
    timeCursor->setTimeDiv(timeDiv);
}

void HantekDSOWidget::scrollShift_valueChanged(int time)
{
    double timeShift = (double)time/scrollShift->maximum();
    qDebug("ShiftTime=%f", timeShift);
    gLBox1->setTimeShift(timeShift);
    timeCursor->setTimeShift(timeShift);
}

void HantekDSOWidget::checkCh1Filter_stateChanged(int filter)
{
    ch1Filter = filter?1:0;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetFilter(ch1Filter, ch2Filter, triggerFilter) < 0)
        {
            qDebug("Error running SetFilter command");
        }
    }
}

void HantekDSOWidget::checkCh2Filter_stateChanged(int filter)
{
    ch2Filter = filter?1:0;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetFilter(ch1Filter, ch2Filter, triggerFilter) < 0)
        {
            qDebug("Error running SetFilter command");
        }
    }
}

void HantekDSOWidget::checkTrigFilter_stateChanged(int filter)
{
    triggerFilter = filter?1:0;
    if (dsoAThread.dsoIO.dsoIsFound())
    {
        if (dsoAThread.dsoIO.dsoSetFilter(ch1Filter, ch2Filter, triggerFilter) < 0)
        {
            qDebug("Error running SetFilter command");
        }
    }
}

void HantekDSOWidget::timeCursor_stateChanged(int cursor)
{
    qDebug("Time cursor=%i", cursor);
    gLBox1->update();
}

void HantekDSOWidget::cursorCh1_stateChanged(int cursor)
{
    qDebug("Channel1 cursor=%i", cursor);
    gLBox1->update();
}


void HantekDSOWidget::cursorCh2_stateChanged(int cursor)
{
    qDebug("Channel2 cursor=%i", cursor);
    gLBox1->update();
}


void HantekDSOWidget::saveSetting()
{
    QSettings *config = new QSettings();

    config->beginGroup( "Setting" );

    // save trigger / time settings
    config->setValue( "timeBase", comboTimeBase->currentIndex() );
    config->setValue( "timeDiv", timeDiv->currentIndex() );
    config->setValue( "triggerSource", comboTriggerSource->currentIndex() );
    config->setValue( "triggerSlope", comboTriggerSlope->currentIndex() );
    config->setValue( "triggerMode", comboTriggerMode->currentIndex() );
    config->setValue( "triggerFilter", checkTrigFilter->isChecked() );

    // channel 1 settings
    config->setValue( "ch1Attenuation", comboAtten1->currentIndex() );
    config->setValue( "ch1Voltage", comboVoltage1->currentIndex() );
    config->setValue( "ch1Coupling", comboCoupling1->currentIndex() );
    config->setValue( "ch1Filter", checkCh1Filter->isChecked() );
    
    // channel 2 settings
    config->setValue( "ch2Attenuation", comboAtten2->currentIndex() );
    config->setValue( "ch2Voltage", comboVoltage2->currentIndex() );
    config->setValue( "ch2Coupling", comboCoupling2->currentIndex() );
    config->setValue( "ch2Filter", checkCh2Filter->isChecked() );
    
    // math channel settings
    config->setValue( "mathType", comboMathType->currentIndex() );
    
    // display frame settings
    config->setValue( "viewMode", comboViewMode->currentIndex() );
    config->setValue( "displayInterpolation", comboDisplayInterpolation->currentIndex() );
    config->setValue( "digitalPhosphor", checkDigitalPhosphor->isChecked() );
    config->setValue( "digitalSound", checkDSound->isChecked() );
    
    // slider settings
    config->setValue( "ch1SliderValue", sliderCh1->value() );
    config->setValue( "ch2SliderValue", sliderCh2->value() );
    config->setValue( "triggerSliderPosition", sliderTriggerPos->value() );
    config->setValue( "triggerSliderValue", sliderTrigger->value() );
    config->setValue( "mathSliderValue", sliderChM->value() );
    
    config->sync();
}

void HantekDSOWidget::readSetting()
{
    QSettings *config = new QSettings;
    
    config->beginGroup( "Setting" );
    
    comboTimeBase->setCurrentIndex( config->value( "timeBase", 6 ).toInt() );
    comboTimeBase_activated( comboTimeBase->currentIndex() );
    timeDiv->setCurrentIndex( config->value( "timeDiv", 0 ).toInt() );
    timeDiv_activated( timeDiv->currentIndex() );
    comboTriggerSource->setCurrentIndex( config->value( "triggerSource", 1 ).toInt() );
    comboTriggerSource_activated( comboTriggerSource->currentIndex() );
    comboTriggerSlope->setCurrentIndex( config->value( "triggerSlope", 0 ).toInt() );
    comboTriggerSlope_activated( comboTriggerSlope->currentIndex() );
    comboTriggerMode->setCurrentIndex( config->value( "triggerMode", 1 ).toInt() );
    comboTriggerMode_activated( comboTriggerMode->currentIndex() );
    checkTrigFilter->setChecked( config->value( "triggerFilter", false ).toBool() );
    checkTrigFilter_stateChanged( checkTrigFilter->isChecked() );
    
    comboAtten1->setCurrentIndex( config->value( "ch1Attenuation", 0 ).toInt() );
    comboAtten1_activated( comboAtten1->currentIndex() );
    comboVoltage1->setCurrentIndex( config->value( "ch1Voltage", 0 ).toInt() );
    comboVoltage1_activated( comboVoltage1->currentIndex() );
    comboCoupling1->setCurrentIndex( config->value( "ch1Coupling", 0 ).toInt() );
    comboCoupling1_activated( comboCoupling1->currentIndex() );
    checkCh1Filter->setChecked( config->value( "ch1Filter", false ).toBool() );
    checkCh1Filter_stateChanged( checkCh1Filter->isChecked() );
    
    comboAtten2->setCurrentIndex( config->value( "ch2Attenuation", 0 ).toInt() );
    comboAtten2_activated( comboAtten2->currentIndex() );
    comboVoltage2->setCurrentIndex( config->value( "ch2Voltage", 0 ).toInt() );
    comboVoltage2_activated( comboVoltage2->currentIndex() );
    comboCoupling2->setCurrentIndex( config->value( "ch2Coupling", 0 ).toInt() );
    comboCoupling2_activated( comboCoupling2->currentIndex() );
    checkCh2Filter->setChecked( config->value( "ch2Filter", false ).toBool() );
    checkCh2Filter_stateChanged( checkCh2Filter->isChecked() );
    
    comboMathType->setCurrentIndex( config->value( "mathType", 0 ).toInt() );
    comboMathType_activated( comboMathType->currentIndex() );

    comboViewMode->setCurrentIndex( config->value( "viewMode", 0 ).toInt() );
    comboViewMode_activated( comboViewMode->currentIndex() );
    comboDisplayInterpolation->setCurrentIndex( config->value( "displayInterpolation", 0 ).toInt() );
    comboDisplayInterpolation_activated( comboDisplayInterpolation->currentIndex() );
    checkDigitalPhosphor->setChecked( config->value( "digitalPhosphor", false ).toBool() );
    checkDigitalPhosphor_stateChanged( checkDigitalPhosphor->isChecked() );
    checkDSound->setChecked( config->value( "digitalSound", false ).toBool() );
    checkDSound_stateChanged( checkDSound->isChecked() );

    sliderCh1->setValue( config->value( "ch1SliderValue", 400 ).toInt() );
    sliderCh2->setValue( config->value( "ch2SliderValue", 400 ).toInt() );
    sliderTriggerPos->setValue( config->value( "triggerSliderPosition", 10239 ).toInt() );
    sliderTrigger->setValue( config->value( "triggerSliderValue", 400 ).toInt() );
    sliderChM->setValue( config->value( "mathSliderValue", 400 ).toInt() );
}
