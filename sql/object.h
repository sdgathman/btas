typedef struct Object {
  struct Object_class {
    int size;
    void (*free)(/**/ struct Object * /**/);
  } *_class;
} Object;

extern struct Object_class Object_class;
#define Object_init(this) (this&&this->_class = &Object_class,this)

#define do0(this,op)		(this)->_class->op(this)
#define do1(this,op,x)		(this)->_class->op(this,x)
#define do2(this,op,x,y)	(this)->_class->op(this,x,y)
#define do3(this,op,x,y,z)	(this)->_class->op(this,x,y,z)
#define new0(type)		type##_init((PTR)0)
#define new1(type,p1)		type##_init((PTR)0,p1)
#define new2(type,p1,p2)	type##_init((PTR)0,p1,p2)
#define new3(type,p1,p2,p3)	type##_init((PTR)0,p1,p2,p3)
