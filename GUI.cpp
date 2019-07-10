#include "pch.h"
#include "GUI.h"
#include "BW_native.h"
#include "Py_config.h"
#include "MyLogger.h"


INIT_LOCAL_MSG_BUFFER;

// Инициализация

bool GUI::inited = false;

long GUI::delLabelCBID = 0;

PyObject* GUI::modGUI = nullptr;


// public methods

MyErr GUI::init()
{
	//загрузка modGUI

	debugLog("Mod_GUI module loading...");

	PyObject* mGUI_module = PyImport_ImportModule(MOD_NAME ".native.mGUI");

	if (!mGUI_module)
		return_err 1;

	debugLog("Mod_GUI class loading...");

	modGUI = PyObject_CallMethod(mGUI_module, "Mod_GUI", nullptr);

	Py_DECREF(mGUI_module);

	if (!modGUI)
		return_err 2;

	debugLog("Mod_GUI class loaded OK!");

	inited = true;

	return_ok;
}

void GUI::fini()
{
	if (!inited)
		return;

	Py_XDECREF(modGUI);
	modGUI = nullptr;
}


PyObject* GUI::getAttr(char* attribute) {
	if (!inited) {
		return nullptr;
	}

	return PyObject_GetAttrString(modGUI, attribute);
}

bool GUI::setAttr(char* attribute, PyObject* value) {
	if (!inited)
		return false;

	return PyObject_SetAttrString(modGUI, attribute, value);
}

MyErr GUI::setWarning(uint8_t warningCode) {
#if debug_log && extended_debug_log
	if (!inited)
		return_err 1;

	auto res = PyObject_CallMethod(modGUI, "setWarning", "b", warningCode);

	Py_XDECREF(res);
#endif

	return_ok;
}

MyErr GUI::setError(uint8_t errorCode) {
#if debug_log && extended_debug_log
	if (!inited)
		return_err 1;

	auto res = PyObject_CallMethod(modGUI, "setError", "b", errorCode);

	Py_XDECREF(res);
#endif

	return_ok;
}

MyErr GUI::setVisible(bool visible) {
	if (!inited)
		return_err 1;

	auto res = PyObject_CallMethod(modGUI, "setVisible", "b", visible);

	Py_XDECREF(res);

	return_ok;
}

MyErr GUI::setTimerVisible(bool visible) {
	if (!inited)
		return_err 1;

	auto res = PyObject_CallMethod(modGUI, "setTimerVisible", "b", visible);

	Py_XDECREF(res);

	return_ok;
}

MyErr GUI::setTime(uint32_t time_preparing) {
	if (!inited)
		return_err 1;

	char new_time[30];

	sprintf_s(new_time, 30, "Time: %02d:%02d", time_preparing / 60, time_preparing % 60);

	auto res = PyObject_CallMethod(modGUI, "setTime", "s", new_time);

	Py_XDECREF(res);

	return_ok;
}

MyErr GUI::setText(char* msg, float time_f) {
	if (!inited)
		return_err 1;

	auto res = PyObject_CallMethod(modGUI, "setText", "s", msg);

	Py_XDECREF(res);

	PyObject* delLabelCBID_p = getAttr("delLabelCBID");

	if (!delLabelCBID_p || delLabelCBID_p == Py_None) {
		delLabelCBID = 0;

		Py_XDECREF(delLabelCBID_p);
	}
	else {
		delLabelCBID = PyInt_AS_LONG(delLabelCBID_p);

		Py_DECREF(delLabelCBID_p);
	}

	if (delLabelCBID) {
		BigWorldUtils::cancelCallback(delLabelCBID);
		delLabelCBID = 0;

		if (!setAttr("delLabelCBID", Py_None)) {
			extendedDebugLogEx(WARNING, "GUI_setText - failed to set delLabelCBID");
		}
	}

	if (time_f) {
		auto res2 = PyObject_CallMethod(modGUI, "clearTextCB", "f", time_f);

		Py_XDECREF(res2);

		delLabelCBID_p = getAttr("delLabelCBID");

		if (!delLabelCBID_p || delLabelCBID_p == Py_None) delLabelCBID = 0;
		else delLabelCBID = PyInt_AsLong(delLabelCBID_p);
	}

	return_ok;
}

MyErr GUI::setMsg(uint8_t msg_ID, float time_f, uint8_t score_ID) {
	if (!inited || msg_ID >= MESSAGES_COUNT || score_ID >= SECTIONS_COUNT)
		return_err 1;

	if (lastStageID == current_map.stageID && lastStageID != STAGE_ID::GET_SCORE && lastStageID != STAGE_ID::ITEMS_NOT_EXISTS)
		return_err 2;

	// получить из словаря локализации нужную строку

	PyObject* messagesList = PyDict_GetItemString(PyConfig::g_self->i18n, "UI_messages");

	if (!messagesList)
		return_err 3;

	//----------------------------------------------

	//находим сообщение из списка

	PyObject* msg_p = PyList_GetItem(messagesList, msg_ID);

	if (!msg_p)
		return_err 4;

	char* msg = PyString_AsString(msg_p);

	if (!msg)
		return_err 5;

	//---------------------------

	char new_msg[255];

	if (msg_ID == STAGE_ID::GET_SCORE) // если это - сообщение о том, что получили баллы
		sprintf_s(new_msg, 255, msg, COLOURS[msg_ID], SCORE[score_ID]);
	else
		sprintf_s(new_msg, 255, msg, COLOURS[msg_ID]);

	setText(new_msg, time_f);

	lastStageID = current_map.stageID;

	return_ok;
}

MyErr GUI::clearText()
{
	if (!inited)
		return_err 1;

	auto res = PyObject_CallMethod(modGUI, "clearText", nullptr);

	Py_XDECREF(res);

	lastStageID = STAGE_ID::COMPETITION;

	delLabelCBID = 0;

	return_ok;
}
