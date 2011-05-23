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
  char *activated_as;
} CryptSetupObject;


int yesDialog(const char *msg, void *this0)
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
    ok = PyArg_Parse(result, "i", &res);
    if(!ok){
      res = 0;
    }

    Py_DECREF(result);
    return res;
  }

  return 1;
}

int passwordDialog(const char *msg, char *buf, size_t length, void *this0)
{
  PyObject *result;
  PyObject *arglist;
  CryptSetupObject *self = (CryptSetupObject *)this0;  
  char *res = NULL;
  int ok;

  if(self->passwordDialogCB){
    arglist = Py_BuildValue("(s)", msg);
    if(!arglist) return 0;
    result = PyEval_CallObject(self->passwordDialogCB, arglist);
    Py_DECREF(arglist);

    if (result == NULL) return 0;

    ok = PyArg_Parse(result, "z", &res);
    fprintf(stderr, "Parsing passw from callback result [%p]: %d %s [%p]\n", result, ok, res, res);

    if(!ok){
      return 0;
    }

    // copy the password
    strncpy(buf, res, length-1);
    Py_DECREF(result);
    fprintf(stderr, "Passphrase received: %s [%d]\n", buf, strlen(buf));

    return strlen(buf);
  }
  else return 0;
}


void cmdLineLog(int cls, const char *msg, void *this0)
{
  PyObject *result;
  PyObject *arglist;
  CryptSetupObject *this = (CryptSetupObject *)this0;

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

  if(self->activated_as) free(self->activated_as);

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
    self->activated_as = NULL;
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
             *passwordDialogCB=NULL,
             *cmdLineLogCB=NULL,
             *tmp=NULL;
    char *device = NULL, *deviceName = NULL;

  static char *kwlist[] = {"device", "name", "yesDialog", "passwordDialog", "logFunc", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zzOOO", kwlist, 
                                   &device, &deviceName,
                                   &yesDialogCB, &passwordDialogCB,
                                   &cmdLineLogCB))
    return -1;

  if (device) {
      if (crypt_init(&(self->device), device)) {
          PyErr_SetString(PyExc_IOError, "Device cannot be opened");
          return -1;
      }
      crypt_load(self->device, NULL, NULL);
      if(deviceName) self->activated_as = strdup(deviceName);
  } else if (deviceName) {
      if (crypt_init_by_name(&(self->device), deviceName)) {
          PyErr_SetString(PyExc_IOError, "Device cannot be opened");
          return -1;
      }
      self->activated_as = strdup(deviceName);
      crypt_load(self->device, NULL, NULL);
  } else {
    PyErr_SetString(PyExc_RuntimeError, "Either device file or luks name has to be specified");
    return -1;
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
    static char *kwlist[] = {"name", "passphrase", NULL};
  PyObject *result;
  int is = 0;
  char *name = NULL;
  char *passphrase = NULL;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|s", kwlist, 
                                    &name, &passphrase))
      return NULL;

  is = crypt_activate_by_passphrase(self->device, name,
                                    CRYPT_ANY_SLOT, passphrase, passphrase ? strlen(passphrase):0, 0);

  if(!is){
      if(self->activated_as) free(self->activated_as);
      self->activated_as = strdup(name);
  }

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

  fprintf(stderr, "deactivating %s [%p]", self->activated_as, self->activated_as);

  is = crypt_deactivate(self->device, self->activated_as);

  if(!is){
      if(self->activated_as) free(self->activated_as);
      self->activated_as = NULL;
  }

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

static PyObject *CryptSetup_isLuks(CryptSetupObject* self, PyObject *args, PyObject *kwds)
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
  PyObject *result;

  result = Py_BuildValue("{s:s,s:s,s:z,s:s,s:s,s:s,s:i,s:K}",
                         "dir", crypt_get_dir(),
                         "device", crypt_get_device_name(self->device),
                         "name", self->activated_as,
                         "uuid", crypt_get_uuid(self->device),
                         "cipher", crypt_get_cipher(self->device),
                         "cipher_mode", crypt_get_cipher_mode(self->device),
                         "keysize", crypt_get_volume_key_size(self->device)*8,
                         //"size", co.size,
                         //"mode", (co.flags & CRYPT_FLAG_READONLY) ? "readonly" : "read/write",
                         "offset", crypt_get_data_offset(self->device)
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
  int is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "|zzO", kwlist, 
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

#define CryptSetup_addKeyByPassphrase_HELP "Initialize keyslot using passphrase\n\
\n\
  device.addKeyByPassphrase(passphrase, newPassphrase, slot)\n\
\n\
  passphrase - string or none to ask the user\n\
  newPassphrase - passphrase to add\n\
  slot - which slot to use (optional)"

static PyObject *CryptSetup_addKeyByPassphrase(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"passphrase", "newPassphrase", "slot", NULL};
  char* passphrase = NULL;
  size_t passphrase_len = 0;
  char* newpassphrase = NULL;
  size_t newpassphrase_len = 0;
  int slot = CRYPT_ANY_SLOT;
  PyObject *result;
  int is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "ss|i", kwlist, 
                                    &passphrase, &newpassphrase, &slot))
    return NULL;

  if(passphrase) passphrase_len = strlen(passphrase);
  if(newpassphrase) newpassphrase_len = strlen(newpassphrase);

  is = crypt_keyslot_add_by_passphrase(self->device, slot, passphrase, passphrase_len, newpassphrase, newpassphrase_len);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}

#define CryptSetup_addKeyByVolumeKey_HELP "Initialize keyslot using cached volume key\n\
\n\
  device.addKeyByVolumeKey(passphrase, newPassphrase, slot)\n\
\n\
  newPassphrase - passphrase to add\n\
  slot - which slot to use (optional)"

static PyObject *CryptSetup_addKeyByVolumeKey(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"newPassphrase", "slot", NULL};
  char* newpassphrase = NULL;
  size_t newpassphrase_len = 0;
  int slot = CRYPT_ANY_SLOT;
  PyObject *result;
  int is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwlist, 
                                    &newpassphrase,&slot))
    return NULL;

  if(newpassphrase) newpassphrase_len = strlen(newpassphrase);

  is = crypt_keyslot_add_by_volume_key(self->device, slot, NULL, 0, newpassphrase, newpassphrase_len);

  result = Py_BuildValue("i", is);
  if(!result){
    PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
    return NULL;
  }

  return result;
}


#define CryptSetup_removePassphrase_HELP "Destroy keyslot using passphrase\n\
\n\
  device.removePassphrase(passphrase)\n\
\n\
  passphrase - string or none to ask the user"

static PyObject *CryptSetup_removePassphrase(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"passphrase", NULL};
  char* passphrase = NULL;
  size_t passphrase_len = 0;
  PyObject *result;
  int is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, 
                                    &passphrase))
      return NULL;
  
  if(passphrase) passphrase_len = strlen(passphrase);
  
  fprintf(stderr, "Passphrase to delete: %s [%d]\n", passphrase, passphrase_len);
  
  is = crypt_activate_by_passphrase(self->device, NULL, CRYPT_ANY_SLOT,
                                   passphrase, passphrase_len, 0);
  if (is < 0)
      goto out;
  
  is = crypt_keyslot_destroy(self->device, is);

 out:
  result = Py_BuildValue("i", is);
  if(!result){
      PyErr_SetString(PyExc_RuntimeError, "Error during constructing values for return value");
      return NULL;
  }

  return result;
}

#define CryptSetup_killSlot_HELP "Destroy keyslot using passphrase\n\
\n\
  device.killSlot(passphrase,slot)\n\
\n\
  passphrase - string or none to ask the user\n\
  slot - the slot to remove"

static PyObject *CryptSetup_killSlot(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"passphrase", "slot", NULL};
  char* passphrase = NULL;
  size_t passphrase_len = 0;
  int slot = -1;
  int slot_status = -1;
  PyObject *result;
  int is;

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "si", kwlist, &passphrase,&slot))
      return NULL;

  if(passphrase) passphrase_len = strlen(passphrase);

  slot_status = crypt_keyslot_status(self->device, slot);
  switch(slot_status){
    case CRYPT_SLOT_INVALID:
        PyErr_SetString(PyExc_ValueError, "Invalid slot");
        return NULL;
    case CRYPT_SLOT_INACTIVE:
        PyErr_SetString(PyExc_ValueError, "Inactive slot");
        return NULL;
	case CRYPT_SLOT_ACTIVE_LAST:
        PyErr_SetString(PyExc_ValueError, "Last slot, removing it would render the device unusable");
        return NULL;
  }
  
  is = crypt_keyslot_destroy(self->device, slot);

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
  
static PyObject *CryptSetup_Status(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  const char* name = self->activated_as;
  PyObject *result;
  crypt_status_info is;

  if(!name){
      PyErr_SetString(PyExc_IOError, "Device has not been activated yet.");
      return NULL;
  }

  is = crypt_status(self->device, name);
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

static PyObject *CryptSetup_Resume(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = {"passphrase", NULL};
  const char* name = self->activated_as;
  char* passphrase = NULL;
  size_t passphrase_len = 0;
  PyObject *result;
  int is;

  if(!name){
      PyErr_SetString(PyExc_IOError, "Device has not been activated yet.");
      return NULL;
  }

  if (! PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, 
                                    &passphrase))
    return NULL;

  if(passphrase) passphrase_len = strlen(passphrase);

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
static PyObject *CryptSetup_Suspend(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  PyObject *result;
  const char* name = self->activated_as;
  int is = 0;

  if(!name){
      PyErr_SetString(PyExc_RuntimeError, "Device has not been activated yet.");
      return NULL;
  }

  is = crypt_suspend(self->device, name);

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
  {"status", (PyCFunction)CryptSetup_Status, METH_NOARGS, CryptSetup_Status_HELP},

  /* cryptsetup mgmt entrypoints */
  {"luksFormat", (PyCFunction)CryptSetup_luksFormat, METH_VARARGS|METH_KEYWORDS, CryptSetup_luksFormat_HELP},
  {"addKeyByPassphrase", (PyCFunction)CryptSetup_addKeyByPassphrase, METH_VARARGS|METH_KEYWORDS, CryptSetup_addKeyByPassphrase_HELP},
  {"addKeyByVolumeKey", (PyCFunction)CryptSetup_addKeyByVolumeKey, METH_VARARGS|METH_KEYWORDS, CryptSetup_addKeyByVolumeKey_HELP},
  {"removePassphrase", (PyCFunction)CryptSetup_removePassphrase, METH_VARARGS|METH_KEYWORDS, CryptSetup_removePassphrase_HELP},
  {"killSlot", (PyCFunction)CryptSetup_killSlot, METH_VARARGS|METH_KEYWORDS, CryptSetup_killSlot_HELP},

  /* suspend resume */
  {"resume", (PyCFunction)CryptSetup_Resume, METH_VARARGS|METH_KEYWORDS, CryptSetup_Resume_HELP},
  {"suspend", (PyCFunction)CryptSetup_Suspend, METH_NOARGS, CryptSetup_Suspend_HELP},


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

