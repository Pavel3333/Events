#include "HangarMessages.h"

PyObject* m_SM_TYPE     = NULL;
PyObject* m_pushMessage = NULL;

//initialization

bool initHangarMessages() {
	//загрузка SM_TYPE и pushMessage

	PyObject* SystemMessages = PyImport_ImportModule("gui.SystemMessages");

	if (!SystemMessages) { traceLog
		return false;
	} traceLog

	PyObject* __SM_TYPE = PyString_FromString("SM_TYPE");

	m_SM_TYPE = PyObject_GetAttr(SystemMessages, __SM_TYPE);

	Py_DECREF(__SM_TYPE);

	if (!m_SM_TYPE) { traceLog
		Py_DECREF(SystemMessages);

		return false;
	} traceLog

	PyObject* __pushMessage = PyString_FromString("pushMessage");

	m_pushMessage = PyObject_GetAttr(SystemMessages, __pushMessage);

	Py_DECREF(__pushMessage);
	Py_DECREF(SystemMessages);

	if (!m_pushMessage) { traceLog
		Py_DECREF(m_SM_TYPE);
		return false;
	} traceLog

	return true;
}

void finiHangarMessages() {
	Py_XDECREF(m_pushMessage);
	Py_XDECREF(m_SM_TYPE);
}

uint8_t showed = false;

uint8_t showMessage(PyObject* i18n) {
	if (!first_check && showed) return NULL;

	PyObject* __GameGreeting = PyString_FromString("GameGreeting");

	PyObject* GameGreeting = PyObject_GetAttr(m_SM_TYPE, __GameGreeting);

	Py_DECREF(__GameGreeting);

	if (!GameGreeting) {
		return 1;
	}

	PyObject* text = NULL;

	if (!first_check) {
		PyObject* __UI_message_thx   = PyString_FromString("UI_message_thx");
		PyObject* __UI_message_thx_2 = PyString_FromString("UI_message_thx_2");

		PyObject* thx_1 = PyObject_GetItem(i18n, __UI_message_thx);
		PyObject* thx_2 = PyObject_GetItem(i18n, __UI_message_thx_2);

		Py_DECREF(__UI_message_thx_2);
		Py_DECREF(__UI_message_thx);

		if (!thx_1 || !thx_2) {
			Py_XDECREF(thx_2);
			Py_XDECREF(thx_1);

			Py_DECREF(GameGreeting);

			return 2;
		}

		char* thx_1_c = PyString_AsString(thx_1);
		char* thx_2_c = PyString_AsString(thx_2);

		if (!thx_1_c || !thx_2_c) {
			Py_DECREF(thx_2);
			Py_DECREF(thx_1);

			Py_DECREF(GameGreeting);

			return 3;
		}

		text = PyUnicode_FromFormat("<font size=\"14\" color=\"#228b22\">%s<br><a href=\"event:https://pavel3333.ru/\">%s</a></font>", thx_1_c, thx_2_c);

		Py_DECREF(thx_2);
		Py_DECREF(thx_1);
	}
	else if (first_check == 2 || first_check == 3 || first_check == 6) {
		PyObject* __UI_err = PyString_FromFormat("UI_err_%d", (uint32_t)first_check);

		PyObject* UI_err = PyObject_GetItem(i18n, __UI_err);

		Py_DECREF(__UI_err);

		if (!UI_err) {
			Py_DECREF(GameGreeting);

			return 4;
		}

		char* UI_err_c = PyString_AsString(UI_err);

		if (!UI_err_c) {
			Py_DECREF(UI_err);

			Py_DECREF(GameGreeting);

			return 5;
		}

		text = PyUnicode_FromFormat("<font size=\"14\" color=\"#ffcc00\">%s</font>", UI_err_c);

		Py_DECREF(UI_err);
	}
	else {
		PyObject* __UI_description = PyString_FromFormat("UI_description");

		PyObject* UI_description = PyObject_GetItem(i18n, __UI_description);

		Py_DECREF(__UI_description);

		if (!UI_description) {
			Py_DECREF(GameGreeting);

			return 3;
		}

		char* UI_description_c = PyString_AsString(UI_description);

		if (!UI_description_c) {
			Py_DECREF(UI_description);

			Py_DECREF(GameGreeting);

			return 5;
		}

		text = PyUnicode_FromFormat("<font size=\"14\" color=\"#ffcc00\">%s: Error %d</font>", UI_description_c, (uint32_t)first_check);

		Py_DECREF(UI_description);
	}

	if (!text) {
		Py_DECREF(GameGreeting);

		return 6;
	}

	PyObject* res = PyObject_CallFunctionObjArgs(m_pushMessage, text, GameGreeting, NULL);

	Py_DECREF(text);

	Py_XDECREF(res);

	Py_DECREF(GameGreeting);

	if (!first_check) showed = true;
	else              showed = false;

	return NULL;
}
