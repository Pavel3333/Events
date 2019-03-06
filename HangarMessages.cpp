#include "HangarMessages.h"
#include "MyLogger.h"
#include "Py_common.h"
#include "Py_config.h"


// Инициализация
bool      HangarMessages::inited        = false;
bool      HangarMessages::showed        = false;
PyObject* HangarMessages::m_SM_TYPE     = nullptr;
PyObject* HangarMessages::m_pushMessage = nullptr;


MyErr HangarMessages::init()
{
	return init_p();
}


void HangarMessages::fini()
{
	if (!inited)
		return;

	Py_XDECREF(m_SM_TYPE);
	Py_XDECREF(m_pushMessage);

	m_SM_TYPE = nullptr;
	m_pushMessage = nullptr;
}


MyErr HangarMessages::showMessage()
{
	if (!inited)
		return_err 1;

	return showMessage_p();
}


//private methods

MyErr HangarMessages::init_p()
{
	//загрузка SM_TYPE и pushMessage

	PyObject* SystemMessages = PyImport_ImportModule("gui.SystemMessages");

	if (!SystemMessages) { traceLog
		return_err 1;
	} traceLog

	m_SM_TYPE = PyObject_GetAttrString(SystemMessages, "SM_TYPE");

	if (!m_SM_TYPE) { traceLog
		Py_DECREF(SystemMessages);

		return_err 2;
	} traceLog

	m_pushMessage = PyObject_GetAttrString(SystemMessages, "pushMessage");

	Py_DECREF(SystemMessages);

	if (!m_pushMessage) { traceLog
		Py_DECREF(m_SM_TYPE);
		return_err 3;
	} traceLog

	inited = true;

	return_ok;
}

MyErr HangarMessages::showCheckMessage_p(PyObject* GameGreeting)
{
	if (!first_check && showed) 
		return_ok;

	PyObject* text = nullptr;

	if (!first_check) {
		PyObject* thx_1 = PyDict_GetItemString(PyConfig::g_self->i18n, "UI_message_thx");
		PyObject* thx_2 = PyDict_GetItemString(PyConfig::g_self->i18n, "UI_message_thx_2");

		if (!thx_1 || !thx_2) {
			Py_XDECREF(thx_2);
			Py_XDECREF(thx_1);

			return_err 1;
		}

		const char* thx_1_c = PyString_AsString(thx_1);
		const char* thx_2_c = PyString_AsString(thx_2);

		Py_DECREF(thx_2);
		Py_DECREF(thx_1);

		if (!thx_1_c || !thx_2_c) 
			return_err 2;

		text = PyUnicode_FromFormat("<font size=\"14\" color=\"#228b22\">%s<br><a href=\"event:https://pavel3333.ru/\">%s</a></font>", thx_1_c, thx_2_c);
	}
	else if (first_check == 2 || first_check == 3 || first_check == 6) {
		PyObject* __UI_err = PyString_FromFormat("UI_err_%d", first_check);

		PyObject* UI_err = PyObject_GetItem(PyConfig::g_self->i18n, __UI_err);

		Py_DECREF(__UI_err);

		if (!UI_err) 
			return_err 3;

		char* UI_err_c = PyString_AsString(UI_err);

		Py_DECREF(UI_err);

		if (!UI_err_c)
			return_err 4;

		text = PyUnicode_FromFormat("<font size=\"14\" color=\"#ffcc00\">%s</font>", UI_err_c);
	}
	else {
		PyObject* UI_description = PyDict_GetItemString(PyConfig::g_self->i18n, "UI_description");

		if (!UI_description)
			return_err 5;

		char* UI_description_c = PyString_AsString(UI_description);

		Py_DECREF(UI_description);

		if (!UI_description_c)
			return_err 6;

		text = PyUnicode_FromFormat("<font size=\"14\" color=\"#ffcc00\">%s: Error %d</font>", UI_description_c, (uint32_t)first_check);
	}

	if (!text)
		return_err 7;

	PyObject_CallFunctionObjArgs_increfed(res, m_pushMessage, text, GameGreeting, NULL);

	Py_DECREF(text);

	Py_XDECREF(res);

	if (!first_check) showed = true;
	else              showed = false;

	return_ok;
}

MyErr HangarMessages::showYoutubeMessage_p(PyObject* GameGreeting)
{
	//вывод сообщения со ссылкой на ютуб-канал

	PyObject* UI_message_channel = PyDict_GetItemString(PyConfig::g_self->i18n, "UI_message_channel");

	if (!UI_message_channel)  return_err 8;

	char* UI_message_channel_c = PyString_AsString(UI_message_channel);

	Py_DECREF(UI_message_channel);

	if (!UI_message_channel_c) return_err 9;

	PyObject* youtubeText = PyUnicode_FromFormat("<font size=\"14\" color=\"#228b22\"><a href=\"event:https://www.youtube.com/c/RAINNVOD\">%s</a></font>", UI_message_channel_c);

	if (!youtubeText) return_err 10;

	PyObject_CallFunctionObjArgs_increfed(res2, m_pushMessage, youtubeText, GameGreeting, NULL);

	Py_DECREF(youtubeText);
	Py_XDECREF(res2);

	return_ok;
}

MyErr HangarMessages::showMessage_p()
{
	PyObject* GameGreeting = PyObject_GetAttrString(m_SM_TYPE, "GameGreeting");

	if (!GameGreeting) {
		return_err 1;
	}

	showCheckMessage_p(GameGreeting);
	showYoutubeMessage_p(GameGreeting);

	Py_DECREF(GameGreeting);

	return_ok;
}
