/** @mainpage BTree Access System
  @copyright 1990,1993,2010 Business Management Systems, Inc
  
 
  The <code>btserve</code> process responds to low level btas operation
  requests.  Both request and response are encapsulated in a <code>BTCB</code>
  struct.  Low level requests deal with a single file or directory record, or
  meta-data for a single file.  Files and directories form a hierarchical
  filesystem not unlike a unix filesystem.  Multiple filesystems may be
  mounted.  They may be mounted on an existing directory (like unix), or on an
  unused mount id (like Windows).  
 
  Mount id 0 is the root filesystem.  All mounted files and directories are
  reachable by starting at the root directory of a filesystem not mounted on a
  directory.  Files and directories are opened at the low level by specifying
  a starting mount id, and a null separated directory path.  There are no
  disallowed characters other than <code>NUL</code>.
 
  The high level text form of an absolute path name identifying a file or
  directory begins with an optional drive specification consisting of a letter
  and colon.  <code>A:</code> is the root filesystem, and is the default when
  no drive letter is specified.  The optional drive specification is followed
  by a slash representing the root directory, and the names of sucessive
  parent directories separated by forward slashes, and finally the name of the
  file or directory in question.  Backslashes may be used to escape forward
  slashes, colons, or themselves to specify an arbitrary low level pathname.
  Control characters other than <code>NUL</code> may be specified as octal or
  hex escapes as in C.  FIXME: drive letters and escapes are not yet
  implemented in routines that deal with high level text path names.
 
  All files and directories are content addressable.  However, there are no
  fields, record length, or key length at the low level.  Records are written
  to a file or directory with the <code>BTWRITE</code> operation.  Each
  write includes a record length (total bytes in the record) and key length
  (the number of leading bytes that must be unique).  The record length
  and key length apply to that operation only.  In particular, the key
  length is never saved.  If the first key length bytes of the record are not
  unique within the file or directory, the operation fails with the 
  <code>BTERDUP</code> result code.  The key length must not be greater than
  the record length.  FIXME: the key length is also limited to MAXKEY bytes,
  currently 256.

  Records are read using the BTREAD opcodes.  Reads specify a record length,
  which is the maximum record size that can be returned, and a key length,
  which is the size of the search data.  The read operation finds the set of
  all records which match the search data for key length leading bytes.
  For read operations depending on ordering of the records, records are
  ordered by an unsigned byte comparison.  When one record is a prefix of 
  another, the shorter record comes first.  ("A" < "AA" but "AA" < "B".)
  There can never be any duplicate records because of the constraints on
  <code>BTWRITE</code>.
  <ul>
  <li> <code>BTREADEQ</code> fails with <code>BTERKEY</code> if the set is
  empty and fails with <code>BTERDUP</code> if the set has more than one
  record.  Otherwise it returns the unique record matching the search data
  and set <code>BTCB.rlen</code> to the size of the return record.
  <li> <code>BTREADF</code> and <code>BTREADL</code> fail with 
  <code>BTERKEY</code> if the set is empty, otherwise they return the first
  or last record of the set respectively.
  <li> <code>BTREADGT</code> and <code>BTREADLT</code> return the smallest
  record larger than any in the set, or the largest record less than any in the
  set, respectively.  They return <code>BTERKEY</code> if there is no such
  record.
  <li> <code>BTREADGE</code> and <code>BTREADLE</code> are equivalent to
  BTREADL or BTREADF followed by BTREADGT or BTREADLT if there are no matching
  records.
  </ul>
 
 */
