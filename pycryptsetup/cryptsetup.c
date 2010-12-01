#include <Python.h>
#include "structmember.h"

#include <stdio.h>
#include <string.h>

#include "libcryptsetup.h"

typedef struct {
  PyObject_HEAD

  /* Type-specific fields go here. */
  PyObject *yesDialogCB;
  PyObject *cmdLineLogCB;
  PyObject *passwordDialogCB;
  struct crypt_device *device;
} CryptSetupObject;


int yesDialog(char *msg, void *this0)
{
  PyObject *result;
  PyObject *arglist;
  int res;
  int ok;
  CryptSetupObject *this = (CryptSetupObject *)this0;

  if(this->yesDialogCB){
    arglist = Py_BuildValue("(s)", msg);
    if(!arglist) return 0;
    result = PyEval_CallObject(this->yesDialogCB, arglist);
    Py_DECREF(arglist);

    if (result == NULL) return 0;
    ok = PyArg_ParseTuple(result, "i", &res);
    if(!ok){
      res = 0;
    }

    Py_DECREF(result);
    return res;
  }
  else return 1;

  return 1;
}

int passwordDialog(char *msg, void *this0)
{
  PyObject *result;
  PyObject *arglist;
  CryptSetupObject *this = (CryptSetupObject *)this0;  
  int res;
  int ok;

  if(this->yesDialogCB){
    arglist = Py_BuildValue("(s)", msg);
    if(!arglist) return 0;
    result = PyEval_CallObject(this->passwordDialogCB, arglist);
    Py_DECREF(arglist);

    if (result == NULL) return 0;
    ok = PyArg_ParseTuple(result, "i", &res);
    if(!ok){
      res = 0;
    }

    Py_DECREF(result);
    return res;
  }
  else return 1;

  return 1;
}


void cmdLineLog(struct crypt_device *cd, int cls, char *msg)
{
  PyObject *result;
  PyObject *arglist;

  if(this->cmdLineLogCB){
    arglist = Py_BuildValue("(is)", cls, msg);
    if(!arglist) return;
    result = PyEval_CallObject(this->cmdLineLogCB, arglist);
    Py_DECREF(arglist);
    Py_XDECREF(result);
  }
}

static void CryptSetup_dealloc(CryptSetupObject* self)
{
  /* free the callbacks */
  Py_XDECREF(self->yesDialogCB);
  Py_XDECREF(self->cmdLineLogCB);
  Py_XDECREF(self->passwordDialogCB);

  /* free self */
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *CryptSetup_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  CryptSetupObject *self;

  self = (CryptSetupObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->yesDialogCB = NULL;
    self->passwordDialogCB = NULL;
    self->cmdLineLogCB = NULL;
  }

  return (PyObject *)self;
}

#define CryptSetup_HELP "CryptSetup object\n\
\n\
constructor takes one to five arguments:\n\
  __init__(device, name, yesDialog, passwordDialog, logFunc)\n\
\n\
  yesDialog - python function with func(text) signature, which asks the user question text and returns 1 of the answer was positive or 0 if not\n\
  logFunc   - python function with func(level, text) signature to log stuff somewhere"

static int CryptSetup_init(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
    PyObject *yesDialogCB=NULL,
             *passwordDialog=NULL,
             *cmdLineLogCB=NULL,
             *tmp=NULL;
    char *device = NULL, *deviceName = NULL;

  static char *kwlist[] = {"device", "name", "yesDialog", "passwordDialog", "logFunc", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ssOOO", kwlist, 
                                   &device, &deviceName,
                                   &yesDialogCB, &passwordDialog,
                                   &cmdLineLogCB))
    return -1;

  if (device) {
      crypt_init(&(self->device), device);
      crypt_load(self->device, NULL, NULL);
  } else if (name) {
      crypt_init_by_name(&(self->device), name);
      crypt_load(self->device, NULL, NULL);
  } else {
      /* TODO XXX error */
  }

  if(yesDialogCB){
    tmp = self->yesDialogCB;
    Py_INCREF(yesDialogCB);
    self->yesDialogCB = yesDialogCB;
    Py_XDECREF(tmp);
    crypt_set_confirm_callback(self->device, yesDialog, self);
  }

  if(passwordDialogCB){
    tmp = self->passwordDialogCB;
    Py_INCREF(passwordDialogCB);
    self->passwordDialogCB = passwordDialogCB;
    Py_XDECREF(tmp);
    crypt_set_password_callback(self->device, passwordDialog, self);
  }
  
  if(cmdLineLogCB){
    tmp = self->cmdLineLogCB;
    Py_INCREF(cmdLineLogCB);
    self->cmdLineLogCB = cmdLineLogCB;
    Py_XDECREF(tmp);
    crypt_set_log_callback(self->device, cmdLineLog, self);
  }

  return 0;
}

#define CryptSetup_activate_HELP "Activate LUKS device\n\
\n\
  activate(name)"

static PyObject *CryptSetup_activate(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"name", NULL};
  PyObject *result;
  int is;
  char *name = NULL;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "z", kwlist, 
                                    &name))


  is = crypt_activate_by_passphrase(self->device, name,
                               CRYPT_ANY_SLOT, NULL, 0, flags);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}

#define CryptSetup_deactivate_HELP "Dectivate LUKS device\n\
\n\
  deactivate()"

static PyObject *CryptSetup_deactivate(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  PyObject *result;
  int is;
  char *passphrase = NULL;

  is = crypt_deactivate(self->device, crypt_get_device_name(self->device));

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}

#define CryptSetup_askyes_HELP "Asks a question using the configured dialog CB\n\
\n\
  int askyes(message)"

static PyObject *CryptSetup_askyes(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"message", NULL};
  PyObject *message=NULL;
  PyObject *result;
  PyObject *arglist;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, 
	&message))
    return NULL;

  Py_INCREF(message);
  arglist = Py_BuildValue("(O)", message);
  if(!arglist){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for internal call");
    return NULL;
  }
  result = PyEval_CallObject(self->yesDialogCB, arglist);
  Py_DECREF(arglist);
  Py_DECREF(message);

  return result;
}

#define CryptSetup_log_HELP "Logs a string using the configured log CB\n\
\n\
  log(int level, message)"

static PyObject *CryptSetup_log(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"priority", "message", NULL};
  PyObject *message=NULL;
  PyObject *priority=NULL;
  PyObject *result;
  PyObject *arglist;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, 
	&message, &priority))
    return NULL;

  Py_INCREF(message);
  Py_INCREF(priority);
  arglist = Py_BuildValue("(OO)", message, priority);
  if(!arglist){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for internal call");
    return NULL;
  }
  result = PyEval_CallObject(self->cmdLineLogCB, arglist);
  Py_DECREF(arglist);
  Py_DECREF(priority);
  Py_DECREF(message);

  return result;
}

#define CryptSetup_luksUUID_HELP "Get UUID of the LUKS device\n\
\n\
  luksUUID(device)"

static PyObject *CryptSetup_luksUUID(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  PyObject *result;
  
  result = Py_BuildValue("s", crypt_get_uuid(self->device));
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}

#define CryptSetup_isLuks_HELP "Is the device LUKS?\n\
\n\
  isLuks()"

static PyObject *CryptSetup_isLuks(CryptSetupObject* self, PyObject *args, PyObOAject *kwds)
{
  PyObject *result;
  int is;

  is = crypt_load(self->device, CRYPT_LUKS1, NULL);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}


#define CryptSetup_Info_HELP "Returns dictionary with info about opened device\n\
Keys:\n\
dir\n\
name\n\
uuid\n\
cipher\n\
cipher_mode\n\
keysize\n\
device\n\
offset\n\
size\n\
skip\n\
mode\n\
"

static PyObject *CryptSetup_Info(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"name", NULL};
  char* device=NULL;
  PyObject *result;
  crypt_status_info is;

  result = Py_BuildValue("{s:s,s:s,s:s,s:i,s:s,s:K,s:K,s:K,s:s}",
                         "dir", crypt_get_dir(),
                         "name", crypt_get_device_name(self->device),
                         "uuid", crypt_get_uuid(self->device),
                         "cipher", crypt_get_cipher(self->device),
                         "cipher_mode", crypt_get_cipher_mode(self->device),
                         "keysize", crypt_get_volume_key_size(self->device)*8,
                         "device", co.device,
                         "offset", crypt_get_data_offset(self->device),
                         "size", co.size,
                         "skip", co.skip,
                         "mode", (co.flags & CRYPT_FLAG_READONLY) ? "readonly" : "read/write"
                         );

  if(!result){
      PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
      return NULL;
  }

  return result;
}

#define CryptSetup_luksFormat_HELP "Format device to enable LUKS\n\
\n\
  luksFormat(cipher = 'aes', cipherMode = 'cbc-essiv:sha256', keysize = 256)\n\
\n\
  cipher - text string to specify cipher\n\
  cipherMode -  (mode-iv:hash_for_iv. probably aes-cbc-essiv:sha256 or aes-xts-plain)\n\
  keysize - key size in bits, cipher must support this."

static PyObject *CryptSetup_luksFormat(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"cipher", "cipherMode", "keysize", NULL};
  char* cipher_mode=NULL;
  char* cipher=NULL;
  int keysize = 256;
  PyObject *keysize_object = NULL;
  PyObject *result;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "zzO", kwlist, 
                                    &cipher, &cipher_mode, &keysize_object))
    return NULL;

  if(!keysize_object || keysize_object==Py_None){
    //use default value
  } else if(!PyInt_Check(keysize_object)){
    PyErr_SetString(PyExc_TypeError, "keysize must be an integer");
    return NULL;
  } else if(PyInt_AsLong(keysize_object) % 8){
    PyErr_SetString(PyExc_TypeError, "keysize must have integer value dividable by 8");
    return NULL;
  } else if(PyInt_AsLong(keysize_object)<=0){
    PyErr_SetString(PyExc_TypeError, "keysize must be positive number bigger than 0");
    return NULL;
  } else{
    keysize = PyInt_AsLong(keysize_object);
  }


  if(!cipher) cipher="aes";
  if(!cipher_mode) cipher_mode="cbc-essiv:sha256";

  is = crypt_format(self->device, CRYPT_LUKS1, cipher, cipher_mode, NULL, NULL, keysize / 8, NULL);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}


#define CryptSetup_Status_HELP "What is the status of Luks subsystem?\n\
\n\
  luksStatus(name)\n\
\n\
  return value:\n\
  - number with luks status"
  
static PyObject *CryptSetup_Status(PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"name", NULL};
  char* device=NULL;
  PyObject *result;
  crypt_status_info is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, 
	&device))
    return NULL;
OA
  is = crypt_status(device);
  result = Py_BuildValue("i", is);
  if(!result){
      PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
      return NULL;
  }
  return result;
}



#define CryptSetup_Resume_HELP "Resume LUKS device\n\
\n\
  luksOpen(passphrase)\n\
\n\
  passphrase - string or none to ask the user"

static PyObject *CryptSetup_Resume(PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"name", "passphrase", NULL};
  char* passphrase=NULL, *name=NULL;
  PyObject *result;
  int is;
  size_t passphrase_len;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|s", kwlist, 
                                    &name, &passphrase))
    return NULL;

  if(passphrase) passphrase_len = len(passphrase);

  is = crypt_resume_by_passphrase(self->device, name, CRYPT_ANY_SLOT, passphrase, passphrase_len);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}

#define CryptSetup_Suspend_HELP "Close LUKS device and remove it from devmapper\n\
\n\
  luksClose(name)\n\
\n\
  the mapping name which should be removed from devmapper."
static PyObject *CryptSetup_Suspend(PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"name", NULL};
  char* device=NULL;
  PyObject *result;
  int is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, 
	&device))
    return NULL;

  is = crypt_suspend(device);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}

static PyMemberDef CryptSetup_members[] = {
  {"yesDialogCB", T_OBJECT_EX, offsetof(CryptSetupObject, yesDialogCB), 0, "confirmation dialog callback"},
  {"cmdLineLogCB", T_OBJECT_EX, offsetof(CryptSetupObject, cmdLineLogCB), 0, "logging callback"},
  {"passwordDialogCB", T_OBJECT_EX, offsetof(CryptSetupObject, passwordDialogCB), 0, "password dialog callback"},
  {NULL}
};

static PyMethodDef CryptSetup_methods[] = {
  /* self-test methods */
  {"log", (PyCFunction)CryptSetup_log, METH_VARARGS|METH_KEYWORDS, CryptSetup_askyes_HELP},
  {"askyes", (PyCFunction)CryptSetup_askyes, METH_VARARGS|METH_KEYWORDS, CryptSetup_log_HELP},

  /* activation and deactivation */
  {"deactivate", (PyCFunction)CryptSetup_deactivate, METH_NOARGS, CryptSetup_deactivate_HELP},
  {"activate", (PyCFunction)CryptSetup_activate, METH_VARARGS|METH_KEYWORDS, CryptSetup_activate_HELP},

  /* cryptsetup info entrypoints */
  {"luksUUID", (PyCFunction)CryptSetup_luksUUID, METH_NOARGS, CryptSetup_luksUUID_HELP},
  {"isLuks", (PyCFunction)CryptSetup_isLuks, METH_NOARGS, CryptSetup_isLuks_HELP},
  {"info", (PyCFunction)CryptSetup_Info, METH_NOARGS, CryptSetup_Info_HELP},  

  /* cryptsetup mgmt entrypoints */
  {"luksFormat", (PyCFunction)CryptSetup_luksFormat, METH_VARARGS|METH_KEYWORDS, CryptSetup_luksFormat_HELP},

  {NULL}  /* Sentinel */
};

static PyTypeObject CryptSetupType = {
  PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
  "cryptsetup.CryptSetup", /*tp_name*/
  sizeof(CryptSetupObject),   /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)CryptSetup_dealloc,        /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  CryptSetup_HELP,     /* tp_doc */
  0,                   /* tp_traverse */
  0,	               /* tp_clear */
  0,                   /* tp_richcompare */
  0,	               /* tp_weaklistoffset */
  0,                   /* tp_iter */
  0,	               /* tp_iternext */
  CryptSetup_methods,             /* tp_methods */
  CryptSetup_members,             /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)CryptSetup_init,      /* tp_init */
  0,                         /* tp_alloc */
  CryptSetup_new,                 /* tp_new */
};


static PyMethodDef cryptsetup_methods[] = {
    {"status", (PyCFunction)CryptSetup_Status, METH_VARARGS|METH_KEYWORDS, CryptSetup_Status_HELP},
    {"resume", (PyCFunction)CryptSetup_Resume, METH_VARARGS|METH_KEYWORDS, CryptSetup_Resume_HELP},
    {"suspend", (PyCFunction)CryptSetup_Suspend, METH_VARARGS|METH_KEYWORDS, CryptSetup_Suspend_HELP},

    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC initcryptsetup(void)
{
  PyObject* m;
  if (PyType_Ready(&CryptSetupType) < 0)
    return;

  m = Py_InitModule3("cryptsetup", cryptsetup_methods, "CryptSetup pythonized API.");
  Py_INCREF(&CryptSetupType);
  PyModule_AddObject(m, "CryptSetup", (PyObject *)&CryptSetupType);
}

