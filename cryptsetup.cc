#include <Python.h>
#include "structmember.h"

#include <stdio.h>

class CryptSetupICB {
  public:
    PyObject *yesDialogCB;
    PyObject *cmdLineLogCB;

    CryptSetupICB(): yesDialogCB(NULL),cmdLineLogCB(NULL) { printf("CryptSetupICB class initialized - %p, %p\n", yesDialogCB, cmdLineLogCB); }
    ~CryptSetupICB();

    int yesDialog(char *msg);
    void cmdLineLog(int cls, char *msg);
};

CryptSetupICB::~CryptSetupICB()
{
  /* free the callbacks */
  Py_XDECREF(this->yesDialogCB);
  Py_XDECREF(this->cmdLineLogCB);
}

int CryptSetupICB::yesDialog(char *msg)
{
  PyObject *result;
  PyObject *arglist;
  int res;
  int ok;

  if(this->yesDialogCB){
    arglist = Py_BuildValue("(s)", msg);
    result = PyEval_CallObject(this->yesDialogCB, arglist);
    Py_DECREF(arglist);

    if (result == NULL) return 0;
    ok = PyArg_ParseTuple(result, "i", &res);
    if(not ok){
      res = 0;
    }

    Py_DECREF(result);
    return res;
  }
  else return 1;

  return 1;
}

void CryptSetupICB::cmdLineLog(int cls, char *msg)
{
  PyObject *result;
  PyObject *arglist;

  if(this->cmdLineLogCB){
    arglist = Py_BuildValue("(is)", cls, msg);
    result = PyEval_CallObject(this->cmdLineLogCB, arglist);
    Py_DECREF(arglist);
    Py_DECREF(result);
  }
}

extern "C" {

typedef struct {
  PyObject_HEAD

  /* Type-specific fields go here. */
  CryptSetupICB icb;
} CryptSetupObject;

static void CryptSetup_dealloc(CryptSetupObject* self)
{
  /* free self */
  self->icb = CryptSetupICB(); //use empty callback object
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *CryptSetup_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  CryptSetupObject *self;

  self = (CryptSetupObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->icb = CryptSetupICB();
    printf(">>> yesDialogCB: %p\n", self->icb.yesDialogCB);
    printf(">>> cmdLineLogCB: %p\n", self->icb.cmdLineLogCB);
  }

  return (PyObject *)self;
}

static int CryptSetup_init(CryptSetupObject* self, PyObject *args, PyObject *kwds)
{
  PyObject *yesDialogCB=NULL, *cmdLineLogCB=NULL, *tmp=NULL;
  static char *kwlist[] = {"yesDialog", "logFunc", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, 
	&yesDialogCB, &cmdLineLogCB))
    return -1;

  printf("<<< yesDialogCB: %p\n>>> yesDialogCB: %p\n", self->icb.yesDialogCB, yesDialogCB);

  if(yesDialogCB){
    tmp = self->icb.yesDialogCB;
    Py_INCREF(yesDialogCB);
    self->icb.yesDialogCB = yesDialogCB;
    Py_XDECREF(tmp);
  }
  
  printf("<<< cmdLineLogCB: %p\n>>> cmdLineLogCB: %p\n", self->icb.cmdLineLogCB, cmdLineLogCB);

  if(cmdLineLogCB){
    tmp = self->icb.cmdLineLogCB;
    Py_INCREF(cmdLineLogCB);
    self->icb.cmdLineLogCB = cmdLineLogCB;
    Py_XDECREF(tmp);
  }

  return 0;
}

static PyObject *CryptSetup_log(CryptSetupObject* self, PyObject *args, PyObject *kwds)
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
    printf("Neco je spatne...");
    return Py_None;
  }
  result = PyEval_CallObject(self->icb.cmdLineLogCB, arglist);
  Py_DECREF(arglist);
  Py_DECREF(message);

  return result;
}

static PyMemberDef CryptSetup_members[] = {
  {NULL}
};

static PyMethodDef CryptSetup_methods[] = {
  {"log", (PyCFunction)CryptSetup_log, METH_VARARGS|METH_KEYWORDS, "Logs a string using the configured log CB"},
  {NULL}  /* Sentinel */
};

static PyTypeObject CryptSetupType = {
  PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
  "cryptsetup.CryptSetup", /*tp_name*/
  sizeof(CryptSetupICB),   /*tp_basicsize*/
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
  "CryptSetup object",     /* tp_doc */
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

} // extern "C"
