//  pysocks.cpp
//
//  Created by Justin Graves on 5/21/15.
//  Copyright (c) 2015 Infegy, Inc. All rights reserved.

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

// Function predefinitions
PyObject * pysocks_send_string(PyObject *self, PyObject *args);
PyObject * pysocks_read_string(PyObject *self, PyObject *args);

// Module method mapping
static PyMethodDef SockMethods[] = {
	{"send_string",  (PyCFunction)pysocks_send_string, METH_VARARGS, "Send a string over a socket (4 byte length then string data)"},
	{"read_string",  (PyCFunction)pysocks_read_string, METH_VARARGS, "Receive a string from a socket (4 byte length then string data)"},
	{NULL, NULL, 0, NULL}
};

// Module initialization
PyMODINIT_FUNC initpysocks(void) {
	PyObject *m;
	m = Py_InitModule("pysocks", SockMethods);
	if (m == NULL)
		return;
}


PyObject * pysocks_send_string(PyObject *self, PyObject *args) {
	PyObject * sock_obj;
	PyObject * data;

	if(!PyArg_UnpackTuple(args, "send_string", 2, 2, &sock_obj, &data))
		return NULL;

	int sock_fd = PyInt_AsLong(sock_obj);
	if(sock_fd < 1) {
		PyErr_SetString(PyExc_ValueError, "First parameter should be a connected socket handle (integer > 0)");
		return NULL;
	}

	int data_length = (int)PyString_Size(data);

	char * send_buffer = (char *)malloc(4+data_length+8);
	memcpy(send_buffer, &data_length, 4);

	if(data_length)
		memcpy(send_buffer+4, PyString_AsString(data), data_length);

	int total_written = 0;

	bool error_occurred = false;

	Py_BEGIN_ALLOW_THREADS

	int length = data_length+4;

	const char * offset = send_buffer;
	
	while(total_written < length) {
		// Apple sets no pipe signal in socket creation, Linux does it with a flag on send
		#ifdef __APPLE__
		ssize_t this_write = send(sock_fd, offset, length-total_written, 0);
		#else
		ssize_t this_write = send(sock_fd, offset, length-total_written, MSG_NOSIGNAL);
		#endif
		
		if(this_write < 0) {
			if(errno == EAGAIN || errno ==  EWOULDBLOCK) {
				this_write = 0;
				usleep(5000);
			} else {
				error_occurred = true;
				break;
			}
		} else {
			total_written += this_write;
			offset += this_write;

			if(total_written < length) {
				usleep(5000);
			}
		}
	}

	Py_END_ALLOW_THREADS

	free(send_buffer);

	if(error_occurred) {
		PyErr_SetString(PyExc_IOError, "socket send error");
		return NULL;
	}
	
	return PyInt_FromLong(total_written);
}

int _read_all(int sock_fd, char * dest, int length) {
	int len_read = 0;

	Py_BEGIN_ALLOW_THREADS
	
	while(len_read < length) {
		int this_read = (int)recv(sock_fd, dest+len_read, length-len_read, 0);
		
		if(this_read < 0) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(5000);
				continue;
			} else {
				len_read = -1;
				break;
			}
		}
		
		if(this_read > 0) {
			len_read += this_read;

			if(len_read < length) {
				usleep(5000);
			}

		} else if(!this_read) {
			// Disconnected
			len_read = -2;
			break;
		}
	}

	Py_END_ALLOW_THREADS
	
	return len_read;
}

PyObject * pysocks_read_string(PyObject *self, PyObject *args) {
	PyObject * sock_obj;

	if(!PyArg_UnpackTuple(args, "read_string", 1, 1, &sock_obj))
		return NULL;

	int sock_fd = PyInt_AsLong(sock_obj);
	if(sock_fd < 1) {
		PyErr_SetString(PyExc_ValueError, "First parameter should be a connected socket handle (integer > 0)");
		return NULL;
	}

	// Length
	int len = -2;
	
	if(_read_all(sock_fd, (char *)&len, 4) <= 0) {
		PyErr_SetString(PyExc_IOError, "socket read error");
		return NULL;
	}
	
	// All done
	if(len == 0)
		return PyString_FromStringAndSize("", 0);
	
	char * str = (char *)malloc(len+8);
	
	if(_read_all(sock_fd, str, len) <= 0) {
		free(str);
		PyErr_SetString(PyExc_IOError, "socket read error");
		return NULL;
	}
	str[len] = 0;

	PyObject * pystr = PyString_FromStringAndSize(str, len);
	free(str);

	return pystr;
}


int main (int argc, char **argv) {
	/* Pass argv[0] to the Python interpreter */
	Py_SetProgramName(argv[0]);

	/* Initialize the Python interpreter.  Required. */
	Py_Initialize();

	/* Add a static module */
	initpysocks();
}
