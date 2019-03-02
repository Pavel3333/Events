#include "GUI.h"
#include "BW_native.h"
#include "Py_config.h"
#include "MyLogger.h"


INIT_LOCAL_MSG_BUFFER;


long delLabelCBID = 0;

PyObject* modGUI = nullptr;


//GUI methods

PyObject* GUI_getAttr(char* attribute) {
	if (!modGUI) {
		return nullptr;
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

	auto res = PyObject_CallMethod(modGUI, "setWarning", "b", warningCode);

	Py_XDECREF(res);
#endif
}

void GUI_setError(uint8_t errorCode) {
#if debug_log && extended_debug_log
	if (!isInited || !modGUI) {
		return;
	}

	auto res = PyObject_CallMethod(modGUI, "setError", "b", errorCode);

	Py_XDECREF(res);
#endif
}

void GUI_setVisible(bool visible) {
	if (!isInited || !modGUI) {
		return;
	}

	auto res = PyObject_CallMethod(modGUI, "setVisible", "b", visible);

	Py_XDECREF(res);
}

void GUI_setTimerVisible(bool visible) {
	if (!isInited || !modGUI) {
		return;
	}

	auto res = PyObject_CallMethod(modGUI, "setTimerVisible", "b", visible);

	Py_XDECREF(res);
}

void GUI_setTime(uint32_t time_preparing) {
	if (!isInited || !modGUI) {
		return;
	}

	char new_time[30];

	sprintf_s(new_time, 30, "Time: %02d:%02d", time_preparing / 60, time_preparing % 60);

	auto res = PyObject_CallMethod(modGUI, "setTime", "s", new_time);

	Py_XDECREF(res);
}

void GUI_setText(char* msg, float time_f) {
	if (!isInited || !modGUI) {
		return;
	}

	auto res = PyObject_CallMethod(modGUI, "setText", "s", msg);

	Py_XDECREF(res);

	PyObject* delLabelCBID_p = GUI_getAttr("delLabelCBID");

	if (!delLabelCBID_p || delLabelCBID_p == Py_None) {
		delLabelCBID = 0;

		Py_XDECREF(delLabelCBID_p);
	}
	else {
		delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

		Py_DECREF(delLabelCBID_p);
	}

	if (delLabelCBID) {
		gBigWorldUtils->cancelCallback(delLabelCBID);
		delLabelCBID = 0;

		if (!GUI_setAttr("delLabelCBID", Py_None)) {
			extendedDebugLogEx(WARNING, "GUI_setText - failed to set delLabelCBID");
		}
	}

	if (time_f) {
		auto res2 = PyObject_CallMethod(modGUI, "clearTextCB", "f", time_f);

		Py_XDECREF(res2);

		delLabelCBID_p = GUI_getAttr("delLabelCBID");

		if (!delLabelCBID_p || delLabelCBID_p == Py_None) delLabelCBID = NULL;
		else delLabelCBID = PyInt_AsLong(delLabelCBID_p);
	}
}

void GUI_setMsg(uint8_t msg_ID, float time_f, uint8_t score_ID) {
	if (!isInited || !modGUI || msg_ID >= MESSAGES_COUNT || score_ID >= SECTIONS_COUNT) {
		return;
	}

	if (lastStageID == current_map.stageID && lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS) {
		return;
	}

	// получить из словаря локализации нужную строку

	PyObject* messagesList = PyDict_GetItemString(PyConfig::g_self->i18n, "UI_messages");

	if (!messagesList) {
		return;
	}

	//----------------------------------------------

	//находим сообщение из списка

	PyObject* msg_p = PyList_GetItem(messagesList, msg_ID);

	if (!msg_p) {
		return;
	}

	char* msg = PyString_AsString(msg_p);

	if (!msg) {
		return;
	}

	//---------------------------

	char new_msg[255];

	if (msg_ID == STAGE_ID::GET_SCORE) { // если это - сообщение о том, что получили баллы
		sprintf_s(new_msg, 255, msg, COLOURS[msg_ID], SCORE[score_ID]);
	}
	else {
		sprintf_s(new_msg, 255, msg, COLOURS[msg_ID]);
	}

	GUI_setText(new_msg, time_f);

	lastStageID = current_map.stageID;
}

void GUI_clearText()
{
	if (!isInited || !modGUI) {
		return;
	}

	auto res = PyObject_CallMethod(modGUI, "clearText", nullptr);

	Py_XDECREF(res);

	lastStageID = STAGE_ID::COMPETITION;

	delLabelCBID = 0;
}
