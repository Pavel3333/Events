#include "GUI.h"

long delLabelCBID = NULL;

PyObject* modGUI = NULL;

//GUI methods

PyObject* GUI_getAttr(char* attribute) {
	if (!modGUI) {
		return NULL;
	}

	return PyObject_GetAttrString(modGUI, attribute);
}

bool GUI_setAttr(char* attribute, PyObject* value) {
	if (!modGUI) {
		return false;
	}

	return PyObject_SetAttrString(modGUI, attribute, value);
}

void GUI_setWarning(uint8_t warningCode) {
#if debug_log && extended_debug_log
	if (!isInited || !modGUI) {
		return;
	}

	PyObject* __setWarning = PyString_FromStringAndSize("setWarning", 10U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setWarning, PyInt_FromSize_t((size_t)warningCode), NULL);

	Py_DECREF(__setWarning);
	Py_XDECREF(res);
#endif
}

void GUI_setError(uint8_t errorCode) {
#if debug_log && extended_debug_log
	if (!isInited || !modGUI) {
		return;
	}

	PyObject* __setError = PyString_FromStringAndSize("setError", 8U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setError, PyInt_FromSize_t((size_t)errorCode), NULL);

	Py_DECREF(__setError);
	Py_XDECREF(res);
#endif
}

void GUI_setVisible(bool visible) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	PyObject* __setVisible = PyString_FromStringAndSize("setVisible", 10U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setVisible, PyBool_FromLong((long)visible), NULL);

	Py_DECREF(__setVisible);
	Py_XDECREF(res);
}

void GUI_setTimerVisible(bool visible) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	PyObject* __setTimerVisible = PyString_FromStringAndSize("setTimerVisible", 15U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setTimerVisible, PyBool_FromLong((long)visible), NULL);

	Py_DECREF(__setTimerVisible);
	Py_XDECREF(res);
}

void GUI_setTime(uint32_t time_preparing) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	char new_time[30];

	sprintf_s(new_time, 30U, "Time: %02d:%02d", time_preparing / 60, time_preparing % 60);

	PyObject* __setTime = PyString_FromStringAndSize("setTime", 7U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setTime, PyString_FromString(new_time), NULL);

	Py_DECREF(__setTime);
	Py_XDECREF(res);
}

void GUI_setText(char* msg, float time_f) {
	if (!isInited || battleEnded || !modGUI) {
		return;
	}

	PyObject* __setText = PyString_FromStringAndSize("setText", 7U);

	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __setText, PyString_FromString(msg), NULL);

	Py_DECREF(__setText);
	Py_XDECREF(res);

	PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

	if (!delLabelCBID_p || delLabelCBID_p == Py_None) {
		delLabelCBID = NULL;

		Py_XDECREF(delLabelCBID_p);
	}
	else {
		delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

		Py_DECREF(delLabelCBID_p);
	}

	if (delLabelCBID) {
		cancelCallback(&delLabelCBID);

		Py_INCREF(Py_None);

		if (!GUI_setAttr("delLabelCBID", Py_None)) {
			return;
		}
	}

	if (time_f) {
		PyObject* __clearTextCB = PyString_FromStringAndSize("clearTextCB", 11U);
		PyObject* res2 = PyObject_CallMethodObjArgs(modGUI, __clearTextCB, PyFloat_FromDouble(time_f), NULL);

		Py_DECREF(__clearTextCB);

		Py_XDECREF(res2);

		PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

		if (!delLabelCBID_p || delLabelCBID_p == Py_None) delLabelCBID = NULL;
		else delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);
	}
}

void GUI_setMsg(uint8_t msgID, uint8_t scoreID, float time_f) {
	if (!isInited || battleEnded || !modGUI || msgID >= MESSAGES_COUNT || scoreID >= MESSAGES_COUNT) {
		return;
	}

	if (lastStageID == current_map.stageID && lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS) {
		return;
	}

	// получить из словаря локализации нужную строку

	PyObject* __UI_messages = PyString_FromStringAndSize("UI_messages", 11U);

	/*if (PyDict_Contains(g_self->i18n, __UI_messages) != 1U) {
		Py_DECREF(__UI_messages);
		return ;
	}*/

	PyObject* messagesList = PyDict_GetItem(g_self->i18n, __UI_messages);

	Py_DECREF(__UI_messages);

	if (!messagesList) {
		return;
	}

	//----------------------------------------------

	//находим сообщение из списка

	PyObject* msg_p = PyList_GetItem(messagesList, msgID);

	if (!msg_p) {
		return;
	}

	char* msg = PyString_AsString(msg_p);

	if (!msg) {
		return;
	}

	//---------------------------

	char new_msg[255];

	if (msgID == STAGE_ID::GET_SCORE) { // если это - сообщение о том, что получили баллы
		sprintf_s(new_msg, 255U, msg, COLOURS[msgID], SCORE[scoreID]);
	}
	else {
		sprintf_s(new_msg, 255U, msg, COLOURS[msgID]);
	}

	GUI_setText(new_msg, time_f);

	lastStageID = current_map.stageID;
}

void GUI_clearText() {
	if (!isInited || !modGUI) {
		return;
	}

	PyObject* __clearText = PyString_FromStringAndSize("clearText", 9U);
	PyObject* res = PyObject_CallMethodObjArgs(modGUI, __clearText, NULL);

	Py_DECREF(__clearText);
	Py_XDECREF(res);

	lastStageID = STAGE_ID::COMPETITION;

	delLabelCBID = NULL;
}
