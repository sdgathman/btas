/* 
 * $Log$
 * Revision 1.2  2003/02/23 03:09:37  stuart
 * More functions.
 *
 * Revision 1.1  2003/02/22 00:50:19  stuart
 * Python interface to cisam emulation.
 *
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
  Py_XDECREF(self->buf);
  Py_XDECREF(self->filename);
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
    case 211: return "File already exists";
  }
  return "Cisam error";
}

/* Throw an exception if a C-isam call failed, otherwise return PyNone. */
static PyObject *
_generic_return(cisam_Object *self,int val) {
  self->isrecnum = isrecnum;
  self->iserrno = iserrno;
  if (val >= 0) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    PyObject *e;
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
  struct btflds *f;
  struct btfrec *fp;
  int fsize = sizeof *f - sizeof f->f + (n+1) * sizeof f->f[0];
  f = (struct btflds *)alloca(fsize);
  if (f == 0) {
    iserrno = ENOMEM;
    return -1;
  }
  f->klen = 2;
  f->rlen = n;
  fp = f->f;
  for (i = 0; i < n; ++i) {
    fp[i].type = *buf++;
    fp[i].len = *buf++;
    fp[i].pos = pos;
    pos += fp[i].len;
  }
  fp[n].type = 0;
  fp[n].len = 0;
  fp[n].pos = pos;
#if 0
  printf("fsize = %d nflds = %d rlen = %d\n",fsize,f->rlen,f->f[f->rlen].pos);
  printf("kflag = %d\n",k->k_flags);
  for (i = 0; i < k->k_nparts; ++i) {
    struct keypart *kp = k->k_part;
    printf("start=%d leng=%d type=%d\n",kp->kp_start,kp->kp_leng,kp->kp_type);
  }
#endif
  i = isbuildx(fname,pos,k,mode,f);
  return i;
}

/** Convert a keydesc represented as a python tuple:
     (flag,[(pos,len,type),...])
 */
static int convertKeydesc(PyObject *kd,void *p) {
  int rc = 0;
  struct keydesc *k = p;
  PyObject *t = PySequence_Fast(kd,"Invalid keydesc");
  if (t == NULL) return 0;
  if (PySequence_Fast_GET_SIZE(t) >= 2) {
    PyObject *flg = PySequence_Fast_GET_ITEM(t,0);
    PyObject *kpart = PySequence_Fast_GET_ITEM(t,1);
    if (PyInt_Check(flg)
      && (kpart = PySequence_Fast(kpart,"Invalid keydesc"))) {
      int n = PySequence_Fast_GET_SIZE(kpart);
      if (n <= NPARTS) {
	int i;
	k->k_flags = (short)PyInt_AS_LONG(flg);
	k->k_nparts = (short)n;
	rc = 1;
	for (i = 0; i < n; ++i) {
	  PyObject *kp = PySequence_Fast_GET_ITEM(kpart,i);
	  kp = PySequence_Tuple(kp);
	  if (kp) {
	    struct keypart *ckp = &k->k_part[i];
	    ckp->kp_type = 0;
	    if (!PyArg_ParseTuple(kp,"hh|h",
		&ckp->kp_start,&ckp->kp_leng,&ckp->kp_type)) rc = 0;
	    Py_DECREF(kp);
	  }
	  else
	    rc = 0;
	}
      }
      Py_DECREF(kpart);
    }
  }
  else
    PyErr_SetString(CisamError,"Invalid keydesc");
  Py_DECREF(t);
  return rc;
}

static PyObject *
Py_FromKeydesc(const struct keydesc *k) {
  PyObject *kd = NULL;
  PyObject *kpart = PyTuple_New(k->k_nparts);
  PyObject *kflags = NULL;
  PyObject *klen = NULL;
  int ok = 1;
  int i;
  if (kpart == NULL) return NULL;
  for (i = 0; i < k->k_nparts; ++i) {
    const struct keypart *kp = &k->k_part[i];
    PyObject *p = Py_BuildValue("iii",kp->kp_start,kp->kp_leng,kp->kp_type);
    if (!p) {
      ok = 0;
      Py_INCREF(Py_None);
      p = Py_None;
    }
    PyTuple_SET_ITEM(kpart,i,p);
  }
  kflags = PyInt_FromLong(k->k_flags);
  klen = PyInt_FromLong(k->k_len);
  if (ok && kflags && klen)
    kd = PyTuple_New(3);
  if (kd == NULL) {
    Py_XDECREF(kflags);
    Py_XDECREF(klen);
    Py_DECREF(kpart);
    return NULL;
  }
  PyTuple_SET_ITEM(kd,0,kflags);
  PyTuple_SET_ITEM(kd,1,kpart);
  PyTuple_SET_ITEM(kd,2,klen);
  return kd;
}

static char cisam_build__doc__[] =
"build(name,keydesc,mode,flds) -> cisam\n\
  Create a new Cisam file.";

static PyObject *
cisam_build(PyObject *module, PyObject *args) {
  char *fname;
  int mode;
  unsigned char *fpbuf;
  int fplen;
  cisam_Object *self;
  struct keydesc kd;
  if (!PyArg_ParseTuple(args, "sO&is#:open",
	&fname,convertKeydesc,(void *)&kd,&mode,&fpbuf,&fplen))
    return NULL;
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
"read(mode) -> None\n\
  Read a logical record.  Mode is one of:\n\
  ISFIRST,ISLAST,ISNEXT,ISPREV,ISCURR,ISGREAT,ISLESS,ISGTEQ,ISLTEQ";

static PyObject *
cisam_read(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  int mode;
  int rc;
  void *buf;
  int len;
  if (!PyArg_ParseTuple(args, "i:read",&mode)) return NULL;
  if (PyObject_AsWriteBuffer(self->buf,&buf,&len)) return NULL;
  rc = isread(self->fd,buf,mode);
  return _generic_return(self,rc);
}

static char cisam_write__doc__[] =
"write(savekey) -> None\n\
  Write a new record.  If savekey is false, make new record current.";

static PyObject *
cisam_write(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  PyObject *flg;
  const void *buf;
  int len;
  int rc;
  if (!PyArg_ParseTuple(args, "O:write",&flg)) return NULL;
  if (PyObject_AsReadBuffer(self->buf,&buf,&len)) return NULL;
  if (PyObject_IsTrue(flg))
    rc = iswrite(self->fd,buf);
  else
    rc = iswrcurr(self->fd,buf);
  return _generic_return(self,rc);
}

static char cisam_rewrite__doc__[] =
"rewrite(recnum) -> None	rewrite by recnum\n\
rewrite(None) -> None	rewrite current record\n\
rewrite() -> None	rewrite by primary key\n\
  Rewrite a record.";

static PyObject *
cisam_rewrite(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  PyObject *flg = 0;
  const void *buf;
  int len;
  int rc;
  if (!PyArg_ParseTuple(args, "|O:write",&flg)) return NULL;
  if (PyObject_AsReadBuffer(self->buf,&buf,&len)) return NULL;
  if (flg == 0)
    rc = isrewrite(self->fd,buf);
  else if (flg == Py_None)
    rc = isrewcurr(self->fd,buf);
  else 
    rc = isrewrec(self->fd,PyInt_AsLong(flg),buf);
  return _generic_return(self,rc);
}

static char cisam_delete__doc__[] =
"delete(recnum) -> None	delete by recnum\n\
delete(None) -> None	delete current record\n\
delete() -> None	delete by primary key\n\
  Rewrite a record.";

static PyObject *
cisam_delete(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  PyObject *flg = 0;
  int rc;
  if (!PyArg_ParseTuple(args, "|O:delete",&flg)) return NULL;
  if (flg == 0) {
    const void *buf;
    int len;
    if (PyObject_AsReadBuffer(self->buf,&buf,&len)) return NULL;
    rc = isdelete(self->fd,buf);
  }
  else if (flg == Py_None)
    rc = isdelcurr(self->fd);
  else 
    rc = isdelrec(self->fd,PyInt_AsLong(flg));
  return _generic_return(self,rc);
}

static char cisam_getfld__doc__[] =
"getfld(pos,len) -> bufptr\n\
  Return a pointer to a fixed length field in the record buffer.";

static PyObject *
cisam_getfld(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  int pos;
  int len;
  if (!PyArg_ParseTuple(args, "ii:getfld",&pos,&len)) return NULL;
  return PyBuffer_FromReadWriteObject(self->buf,pos,len);
}

static char cisam_indexinfo__doc__[] =
"indexinfo(idx) -> keydesc\n\
indexinfo(0) -> dictinfo\n\
  Return keydesc or dictinfo.";

static PyObject *
cisam_indexinfo(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  int idx;
  struct keydesc kd;
  int rc;
  if (!PyArg_ParseTuple(args, "i:indexinfo",&idx)) return NULL;
  if (idx == 0) {
    struct dictinfo dict;
    rc = isindexinfo(self->fd,(struct keydesc *)&dict,0);
    if (rc < 0) return _generic_return(self,rc);
    return Py_BuildValue("iiil",
	dict.di_nkeys,dict.di_recsize,dict.di_idxsize,dict.di_nrecords);
  }
  rc = isindexinfo(self->fd,&kd,idx);
  if (rc < 0) return _generic_return(self,rc);
  return Py_FromKeydesc(&kd);
}

static char cisam_addflds__doc__[] =
"addflds(buf) -> None\n\
  Append additional fields to record.";

static PyObject *
cisam_addflds(PyObject *isamfile, PyObject *args) {
  cisam_Object *self = (cisam_Object *)isamfile;
  char *buf;
  int len;
  int n;
  int i;
  struct btfrec *f;
  if (!PyArg_ParseTuple(args, "s#:indexinfo",&buf,&len)) return NULL;
  n = len/2;
  f = alloca(sizeof *f * n);
  if (!f) return PyErr_NoMemory();
  for (i = 0; i < n; ++i) {
    f[i].type = *buf++;
    f[i].len = *buf++;
  }
  i = isaddflds(self->fd,f,n);
  n = isreclen(self->fd);
  if (n != self->rlen) {
    self->rlen = n;
    Py_XDECREF(self->buf);
    self->buf = PyBuffer_New(n);
  }
  return _generic_return(self,i);
}

static PyMethodDef isamfile_methods[] = {
  { "read",    cisam_read,    METH_VARARGS, cisam_read__doc__},
  { "write",   cisam_write,   METH_VARARGS, cisam_write__doc__},
  { "rewrite", cisam_rewrite, METH_VARARGS, cisam_rewrite__doc__},
  { "delete",  cisam_delete,  METH_VARARGS, cisam_delete__doc__},
  { "getfld",  cisam_getfld,  METH_VARARGS, cisam_getfld__doc__},
  { "indexinfo",cisam_indexinfo,METH_VARARGS, cisam_indexinfo__doc__},
  { "addflds", cisam_addflds, METH_VARARGS, cisam_addflds__doc__},
  { "close",   cisam_close,   METH_VARARGS, cisam_close__doc__},
  { NULL, NULL }
};

static int cmprec(const void *a,const void *b) {
  const struct btfrec *fa = a;
  const struct btfrec *fb = b;
  return fa->pos - fb->pos;
}

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
  if (strcmp(name,"recnum") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    return Py_BuildValue("i",self->isrecnum);
  }
  if (strcmp(name,"iserrno") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    return Py_BuildValue("i",self->iserrno);
  }
  if (strcmp(name,"rlen") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    return Py_BuildValue("i",self->rlen);
  }
  if (strcmp(name,"fldtbl") == 0) {
    cisam_Object *self = (cisam_Object *)isamfile;
    struct btflds *f = isflds(self->fd);
    int i, n;
    struct btfrec *fa;
    PyObject *s;
    char *buf;
    if (!f) {
      iserrno = ENOTOPEN;
      return _generic_return(self,-1);
    }
    // sort by position, first copy 
    n = f->rlen;
    fa = alloca(sizeof *fa * n);
    if (!fa) return PyErr_NoMemory();
    s = PyString_FromStringAndSize(NULL,n*2);
    if (s == NULL) return NULL;
    buf = PyString_AS_STRING(s);
    for (i = 0; i < n; ++i)
      fa[i] = f->f[i];
    qsort(fa,n,sizeof *fa,cmprec);
    for (i = 0; i < n; ++i) {
      *buf++ = fa[i].type;
      *buf++ = fa[i].len;
    }
    return s;
  }
  return Py_FindMethod(isamfile_methods, isamfile, name);
}

static PyMethodDef cisam_methods[] = {
   { "isopen",            cisam_open,        METH_VARARGS, cisam_open__doc__},
   { "isbuild",           cisam_build,       METH_VARARGS, cisam_build__doc__},
   { "iserase",           cisam_erase,       METH_VARARGS, cisam_erase__doc__},
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
   CisamError = PyErr_NewException("cisam.error", PyExc_EnvironmentError, NULL);
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
   CONST(BT_CHAR); CONST(BT_DATE); CONST(BT_TIME); CONST(BT_BIN);
   CONST(BT_PACK); CONST(BT_RECNO); CONST(BT_FLT); CONST(BT_RLOCK);
   CONST(BT_SEQ); CONST(BT_BITS); CONST(BT_EBCDIC); CONST(BT_REL);
   CONST(BT_VNUM); CONST(BT_NUM);
}
