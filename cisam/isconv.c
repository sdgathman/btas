#include "cisam.h"
#include <block.h>
#include <string.h>

/* Use a C-isam keydesc to convert a master field table to the field
   table for a key record implementing the keydesc */

/* will crash if more than MAXFLDS converted */

/* master field table must cover entire record or results are undefined */

/* Method: create a field map that tags each byte of the unpacked record with
   the field number describing it.  Rearrange the field map according
   to the keydesc, then convert it back to a field table.
   Note that adjacent rearrangements of the same field will still be
   one field in the output.  Since this is undesirable, record positions
   are tagged with both field index and position within field. */

/* isrecnum is conceptually a non-contiguous part of the unpacked record.
   It is positioned just after the C-isam portion of the record - i.e.
   the unpacked record is extended internally to include isrecnum. */

/* permute fmap according to keydesc */
static int kdpermute(struct keydesc kd,char *fmap,int size) {
  register int klen = 0;
  register int i,j;
  for (i = 0; i < kd.k_nparts; ++i) {
    int pos = kd.k_part[i].kp_start;
    int len = kd.k_part[i].kp_leng;
    if (pos > klen) {
      rotate(fmap + klen * size, (pos - klen + len) * size, len * size);
      for (j = i + 1; j < kd.k_nparts; ++j)
	if (kd.k_part[j].kp_start < pos) kd.k_part[j].kp_start += len;
    }
    klen += len;
  }
  return klen;
}

/* inverse permutation */
static int kdinvperm(struct keydesc kd,char *fmap,int size) {
  register int klen = 0;
  register int i,j;
  for (i = 0; i < kd.k_nparts; ++i) {
    int len = kd.k_part[i].kp_leng;
    for (j = i + 1; j < kd.k_nparts; ++j)
      if (kd.k_part[j].kp_start < kd.k_part[i].kp_start)
	kd.k_part[j].kp_start += len;
    klen += len;
  }
  kd.k_len = klen;
  while (--i >= 0) {
    int pos = kd.k_part[i].kp_start;
    int len = kd.k_part[i].kp_leng;
    klen -= len;
    if (pos > klen)
      rotate(fmap + klen*size, (pos - klen + len)*size, (pos - klen)*size);
  }
  return kd.k_len;
}

/* convert keydesc to btflds */
struct btflds *isconvkey(const struct btflds *m,struct keydesc *k,int dir) {
  register int i;
  register struct btflds *f;		/* output field table */
  register struct btfrec *fp;		/* field pointer */
  register const struct btfrec *cfp;
  struct keydesc kr;
  struct fmap_st {
    unsigned char mfld;	/* master field index */
    unsigned char fpos;	/* position within master field */
  };	/* field map */
  int rlen = m->f[m->rlen].pos;		/* unpacked master record size */
  struct fmap_st *fmap;
  int klen;				/* unpacked key record size */
  int fcnt;				/* output field count */

  /* ensure fmap big enough for keydesc */
  klen = rlen;
  for (i = 0; i < k->k_nparts; ++i) {
    int len = k->k_part[i].kp_start + k->k_part[i].kp_leng;
    if (len > klen)
      klen = len;
  }
  /* allocate 4 bytes extra in case we need RECNO */
  fmap = alloca((klen + 4) * sizeof *fmap);
  /* initialize field map */

  for (cfp = m->f; cfp->type; ++cfp) {
    for (i = 0; i < cfp->len; ++i) {
      fmap[i+cfp->pos].mfld = cfp - m->f;
      fmap[i+cfp->pos].fpos = i;
    }
  }

  /* convert isrecnum keydesc */

  if (k->k_nparts == 0) {
    kr.k_flags = 0;
    kr.k_nparts = 1;
    kr.k_part->kp_start = isrlen(m);	/* isrecnum position at isam reclen */
    k->k_len = kr.k_part->kp_leng = 4;
    kr.k_part->kp_type = CHARTYPE;
    k = &kr;
#if 0
    for (i = 0; i < 4; ++i) {
      fmap[kr.k_part->kp_start + i].mfld = m->rlen;
      fmap[kr.k_part->kp_start + i].fpos = i;
    }
#endif
  }

  /* rearrange fmap by keydesc */

  if (dir < 0)
    k->k_len = kdinvperm(*k,(char *)fmap,sizeof fmap[0]);
  else
    k->k_len = kdpermute(*k,(char *)fmap,sizeof fmap[0]);

  /* count transitions to allocate output table */

  fcnt = 2;
  for (i = 1; i < rlen; ++i)
    if ((fmap[i].mfld != fmap[i-1].mfld || fmap[i].fpos-1 != fmap[i-1].fpos)
      && (i < k->k_len || fmap[i].mfld < m->klen)) ++fcnt;

  f = (struct btflds *)malloc(sizeof *f - sizeof f->f + fcnt * sizeof *f->f);
  if (f == 0) return f;

  /* begin conversion of fmap to output field table and output first field */

  f->rlen = --fcnt;
  f->klen = 1;
  fp = f->f;

  fp->pos = m->f[fmap->mfld].pos + fmap->fpos;
  fp->type = m->f[fmap->mfld].type;
  fp->len = 1;

  /* build remaining output fields - counting key fields */

  i = 0;
  klen = 0;
  while (++i < rlen) {
    if (i < k->k_len || fmap[i].mfld < m->klen) {
      if (fmap[i].mfld!=fmap[i-1].mfld || fmap[i].fpos-1!=fmap[i-1].fpos) {	
	klen += (fp++)->len;
	fp->pos = m->f[fmap[i].mfld].pos + fmap[i].fpos;
	fp->len = 0;
	fp->type = m->f[fmap[i].mfld].type;
	if (!fp->type)
	  fp->type = BT_RECNO;
	if (i < k->k_len) ++f->klen;		/* count key fields */
      }
      ++fp->len;				/* still same field */
    }
  }
  klen += (fp++)->len;

  fp->pos = klen;
  fp->len = fp->type = 0;

  if (k->k_flags & ISDUPS)
    f->klen = f->rlen;
  
  return f;
}

/* FIXME: it might be better to convert key records directly from
   the btas record buffer and dispense with struct fisam. */

void isstkey(const char *fname,const struct keydesc *k,struct fisam *rec) {
  register int i;
  stchar(fname,rec->name,sizeof rec->name);
  rec->flag = k->k_flags;
  rec->nparts = k->k_nparts;
  for (i = 0; i < k->k_nparts; ++i) {
    stshort(k->k_part[i].kp_start,rec->kpart[i].pos);
    stshort(k->k_part[i].kp_leng,rec->kpart[i].len);
    stshort(k->k_part[i].kp_type,rec->kpart[i].type);
  }
  if (i < NPARTS)
    (void)memset((char *)&rec->kpart[i],0,sizeof *rec->kpart * (NPARTS - i));
}

void isldkey(char *fname,struct keydesc *k,const struct fisam *rec) {
  register int i;
  ldchar(rec->name,sizeof rec->name,fname);
  k->k_flags = rec->flag;
  k->k_nparts = rec->nparts;
  for (i = 0; i < k->k_nparts; ++i) {
    struct keypart *kpp = k->k_part + i;
    kpp->kp_start = ldshort(rec->kpart[i].pos);
    kpp->kp_leng  = ldshort(rec->kpart[i].len);
    kpp->kp_type  = ldshort(rec->kpart[i].type);
  }
}

/* compute C-isam record length as length of contiguous portion of record */

int isrlen(const struct btflds *m) {
  int i, rnumlen = 0;
  if (!m) return 0;
  for (i = 0; i < m->rlen; ++i) {
    if (m->f[i].type == BT_RECNO)
      rnumlen += m->f[i].len;
  }
  return m->f[i].pos - rnumlen;	/* no isrecnum, isrlen == btrlen */
}
