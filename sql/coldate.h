#define Date Column
#define Julian Column
#define Julmin Column
#define Julminp Column
typedef struct Time Time;
extern Column *Date_init(Column *, char *);
extern Column *Julian_init(Column *, char *);
extern Column *Julmin_init(Column *, char *);
extern Column *Julminp_init(Column *, char *);
extern Column *Time_init(Time *, char *,int);
extern Column *Time_init_mask(Time *, char *, const char *);
#define DAYCENTORG (2378496L + 36889L) /* julian day origin of day of century */
