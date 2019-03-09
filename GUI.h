#pragma once

#include "Py_config.h"

class GUI {
public:
	static bool inited;

	static PyObject* modGUI;

	static long delLabelCBID;

	static MyErr init();
	static void fini();

	static PyObject* getAttr(char*);
	static bool      setAttr(char*, PyObject*);

	static void setWarning(uint8_t);
	static void setError(uint8_t);
	static void setVisible(bool);
	static void setTimerVisible(bool);
	static void setTime(uint32_t);
	static void setText(char*, float time_f = 0.0f);
	static void setMsg(uint8_t, float time_f = 0.0f, uint8_t scoreID = 0);
	static void clearText();
};