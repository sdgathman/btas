/* 
 * $Log$
 */

#include <pthread.h>
#include <netinet/in.h>
#include <Python.h>
#include <isamx.h>
#include <btflds.h>

static PyObject *CisamError;

staticforward PyTypeObject cisam_Type;

typedef struct {
  PyObject_HEAD
  int fd;		/* libmilter thread state */
  int rlen;
  int iserrno;
  int isrecnum;		/* ISAM record number of last call */
  PyObject *buf;
  PyObject *filename;
} cisam_Object;

static void
cisam_dealloc(PyObject *s) {
  cisam_Object *self = (cisam_Object *)s;
  int fd = self->fd;
  if (fd >= 0)
    isclose(fd);
  Py_DECREF(self->buf);
  Py_DECREF(self->filename);
}

static const char *
iserrstr(int err) {
  switch (err) {
    case EDUPL: return "duplicate record";
    case ENOTOPEN: return "file not open";
    case EBADARG: return "illegal argument";
    case EBADKEY: return "illegal key desc";
    case ETOOMANY: return "too many files open";
    case EBADFILE: return "bad isam file format";
    case ENOTEXCL: return "non-exclusive access";
    case ELOCKED: return "record locked";
    case EKEXISTS: return "key already exists";
    case EPRIMKEY: return "is primary key";
    case EENDFILE: return "end/begin of file";
    case ENOREC: return "no record found";
    case ENOCURR: return "no current record";
    case EFLOCKED: return "file locked";
    case EFNAME: return "file name too long";
    case ENOLOK: return "can't create lock file";
    case EBADMEM: return "can't alloc memory";
  }
  return "Cisam error";
}

/* Throw an exception if an smfi call failed, otherwise return PyNone. */
static PyObject *
_generic_return(cisam_Object *self,int val) {
  if (val == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    PyObject *e;
    self->iserrno = iserrno;
    e = Py_BuildValue("isO", iserrno,iserrstr(iserrno),self->filename);
    if (e) PyErr_SetObject(CisamError, e);
    return NULL;
  }
}

static PyObject *
_create_return(cisam_Object *self) {
  if (self->fd < 0) {
    PyObject *e = Py_BuildValue("isO",iserrno,iserrstr(iserrno),self->filename);
    Py_DECREF(self);
    if (e) PyErr_SetObject(CisamError,e);
    return NULL;
  }
  self->rlen = isreclen(self->fd);
  self->buf = PyBuffer_New(self->rlen);
  if (self->buf == 0) {
    Py_DECREF(self);
    return NULL;
  }
  return (PyObject *)self;
}

static cisam_Object *
isamfile_New(const char *fname) {
  cisam_Object *self = PyObject_New(cisam_Object,&cisam_Type);
  if (!self) return NULL;
  self->fd = -1;
  self->rlen = 0;
  self->buf = 0;
  self->isrecnum = 0;
  self->iserrno = 0;
  self->filename = Py_BuildValue("s",fname);
  if (self->filename == 0) {
    Py_DECREF(self);
    return NULL;
  }
  return self;
}

static char cisam_erase__doc__[] =
"erase(name) -> cisam\n\
  Open a Cisam file.";

static PyObject *
cisam_erase(PyObject *module, PyObject *args) {
  char *fname;
  if (!PyArg_ParseTuple(args, "s:erase",&fname)) return NULL;
  if (iserase(fname) < 0) {
    PyObject *e = Py_BuildValue("iss",iserrno,iserrstr(iserrno),fname);
    if (e) PyErr_SetObject(CisamError,e);
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static char cisam_open__doc__[] =
"open(name,mode) -> cisam\n\
  Open a Cisam file.";

static PyObject *
cisam_open(PyObject *module, PyObject *args) {
  char *fname;
  int mode;
  cisam_Object *self;
  if (!PyArg_ParseTuple(args, "si:open",&fname,&mode)) return NULL;
  self = isamfile_New(fname);
  if (!self) return NULL;
  self->fd = isopen(fname,mode);
  return _create_return(self);
}

/** Create new isam file from field list and keydesc.  The btflds and
 * record length are computed from the field list.
 */
static int
isbuildfp(const char *fname,struct keydesc *k,int mode,
  const char *buf,int fplen) {
  int n = fplen/2;
  int i;
  int pos = 0;
  struct btflds *f = (struct btflds *)alloca(
	      sizeof *f - sizeof f->f + (n+1) * sizeof f->f[0]);
  struct btfrec *fp = f->f;
  f->klen = 0;
  f->rlen = n;
  for (i = 0; i < n; ++i) {
    fp[i].type = *buf++;
    fp[i].len = *buf++;
    pos += fp[i].len;
  }
  fp[n].type = 0;
  fp[n].len = 0;
  fp[n].pos = pos;
  return isbuildx(fname,pos,k,mode,f);
}

static void
ldkeydesc(struct keydesc *k,const char *p) {
  int i;
  k->k_flags = ldshort(p), p += 2;
  k->k_nparts = ldshort(p), p += 2;
  for (i = 0; i < k->k_nparts; ++i) {
    k->k_part[i].kp_start = ldshort(p), p += 2;
    k->k_part[i].kp_leng = ldshort(p), p += 2;
    k->k_part[i].kp_type = ldshort(p), p += 2;
  }
}

static char cisam_build__doc__[] =
"build(name,keydesc,mode,flds) -> cisam\n\
  Create a new Cisam file.";

static PyObject *
cisam_build(PyObject *module, PyObject *args) {
  char *fname;
  char *kdbuf;
  int kdlen;
  int mode;
  unsigned char *fpbuf;
  int fplen;
  cisam_Object *self;
  struct keydesc kd;
  if (!PyArg_ParseTuple(args, "ss#is#:open",
	&fname,&kdbuf,&kdlen,&mode,&fpbuf,&fplen))
    return NULL;
  if (kdlen < 4 + ldshort(kdbuf + 2) * 6) {
    PyErr_SetString(CisamError,"invalid keydesc");
    return NULL;
  }
  ldkeydesc(&kd,kdbuf);
  self = isamfile_New(fname);
  if (!self) return NULL;
  self->fd = isbuildfp(fname,&kd,mode,fpbuf,fplen);
  return _create_return(self);
}

static char cisam_close__doc__[] =
"close() -> None\n\
  Close the Cisam file attached to this Cisam object.";

static PyObject *
cisam_close(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  int fd = self->fd;
  if (!PyArg_ParseTuple(args, ":close")) return NULL;
  self->fd = -1;
  return _generic_return(self,isclose(fd));
}

static char cisam_read__doc__[] =
"read(key,mode) -> buf\n\
  Read a logical record.  Mode is one of:\n\
  ISFIRST,ISLAST,ISNEXT,ISPREV,ISCURR,ISGREAT,ISLESS,ISGTEQ,ISLTEQ";

static PyObject *
cisam_read(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  int mode;
  int rc;
  char *buf;
  int len;
  if (!PyArg_ParseTuple(args, "i:read",&mode)) return NULL;
  if (PyObject_AsWriteBuffer(self->buf,&buf,&len)) return NULL;
  rc = isread(self->fd,buf,mode);
  return _generic_return(self,rc);
}

static PyMethodDef isamfile_methods[] = {
  { "read",    cisam_read,    METH_VARARGS, cisam_read__doc__},
  { "close",   cisam_close,   METH_VARARGS, cisam_close__doc__},
  { NULL, NULL }
};

static PyObject *
cisam_getattr(PyObject *isamfile, char *name) {
  if (strcmp(name,"recbuf") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    Py_INCREF(self->buf);
    return self->buf;
  }
  if (strcmp(name,"filename") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    Py_INCREF(self->filename);
    return self->filename;
  }
  if (strcmp(name,"indexinfo") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    struct dictinfo dict;
    int rc = isindexinfo(self->fd,(struct keydesc *)&dict,0);
    if (rc < 0) return _generic_return(self,rc);
    return Py_BuildValue("iiil",
	dict.di_nkeys,dict.di_recsize,dict.di_idxsize,dict.di_nrecords);
  }
  if (strcmp(name,"rlen") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    return Py_BuildValue("i",self->rlen);
  }
  return Py_FindMethod(isamfile_methods, isamfile, name);
}

static PyMethodDef cisam_methods[] = {
   { "open",            cisam_open,        METH_VARARGS, cisam_open__doc__},
   { "build",           cisam_build,       METH_VARARGS, cisam_build__doc__},
   { "erase",           cisam_erase,       METH_VARARGS, cisam_erase__doc__},
   { NULL, NULL }
};

static PyTypeObject cisam_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "isamfile",
  sizeof(cisam_Object),
  0,
        cisam_dealloc,            /* tp_dealloc */
        0,               /* tp_print */
        cisam_getattr,           /* tp_getattr */
        0,			/* tp_setattr */
        0,                                      /* tp_compare */
        0,                 /* tp_repr */
        0,                     /* tp_as_number */
        0,                                      /* tp_as_sequence */
        0,                                      /* tp_as_mapping */
        0,                 /* tp_hash */
        0,                                      /* tp_call */
        0,                  /* tp_str */
        0,                                      /* tp_getattro */
        0,                                      /* tp_setattro */
        0,                                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                     /* tp_flags */
};

static char cisam_documentation[] =
"This module interfaces with the BMS Cisam emulator for BTAS/X,\n\
allowing one to access BTAS/X files directly in Python.\n";

void
initcisam(void) {
   PyObject *m, *d;

   m = Py_InitModule4("cisam", cisam_methods, cisam_documentation,
		      (PyObject*)NULL, PYTHON_API_VERSION);
   d = PyModule_GetDict(m);
   CisamError = PyErr_NewException("cisam.error", NULL, NULL);
   PyDict_SetItemString(d,"error", CisamError);
#define CONST(n) PyDict_SetItemString(d,#n, PyInt_FromLong((long) n))
   CONST(ISFIRST); CONST(ISLAST);
   CONST(ISNEXT); CONST(ISPREV);
   CONST(ISCURR); CONST(ISEQUAL);
   CONST(ISGREAT); CONST(ISGTEQ);
   CONST(ISLESS); CONST(ISLTEQ);
   CONST(ISLOCK); CONST(ISWAIT);
   CONST(ISFULL); CONST(ISLCKW);
   CONST(ISINPUT); CONST(ISOUTPUT);
   CONST(ISINOUT); CONST(ISTRANS);
   CONST(ISDIROK); CONST(ISAUTOLOCK);
   CONST(ISMANULOCK); CONST(ISEXCLLOCK);
   CONST(READONLY); CONST(UPDATE);
}
