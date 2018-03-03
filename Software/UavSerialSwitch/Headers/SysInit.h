#ifndef HEADERS_SYSINIT_H_
#define HEADERS_SYSINIT_H_

/*!
* \fn void SysInit_TaskEntry(void* param)
* \brief Reads config file and creates all other tasks afterwards (because other tasks need config file)
*/
void SysInit_TaskEntry(void* param);

#endif
