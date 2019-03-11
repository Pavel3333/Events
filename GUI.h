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

	static MyErr setWarning(uint8_t);
	static MyErr setError(uint8_t);
	static MyErr setVisible(bool);
	static MyErr setTimerVisible(bool);
	static MyErr setTime(uint32_t);
	static MyErr setText(char*, float time_f = 0.0f);
	static MyErr setMsg(uint8_t, float time_f = 0.0f, uint8_t scoreID = 0);
	static MyErr clearText();
};