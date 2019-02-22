#pragma once

#include "Py_config.h"
#include "BW_native.h"

extern PyObject* modGUI;

extern long delLabelCBID;

PyObject* GUI_getAttr(char*);
bool      GUI_setAttr(char*, PyObject*);

void GUI_setWarning(uint8_t);
void GUI_setError(uint8_t);
void GUI_setVisible(bool);
void GUI_setTimerVisible(bool);
void GUI_setTime(uint32_t);
void GUI_setText(char*, float time_f = NULL);
void GUI_setMsg(uint8_t, uint8_t scoreID = NULL, float time_f = NULL);
void GUI_clearText();