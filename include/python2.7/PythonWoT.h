/**
* This file is part of the XVM project.
*
* Copyright (c) 2017 XVM contributors.
*
* This file is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation, version 3.
*
* This file is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>

DWORD WOTPYTHON_GetModuleSize(const wchar_t* lpFilename);
BOOL WOTPYTHON_DataCompare(const char* pData, const char* bMask, const char * szMask);
DWORD WOTPYTHON_FindFunction(DWORD startpos, DWORD endpos, DWORD* curpos, const char* pattern, const char* mask);
DWORD WOTPYTHON_FindStructure(DWORD address, DWORD offset);

DWORD WOTPYTHON_GetFunctionRealAddress(const char* str);
DWORD WOTPYTHON_GetStructureRealAddress(const char* str);
DWORD WOTPYTHON_GetRealAddress(const char* str);

#ifdef __cplusplus
}
#endif
