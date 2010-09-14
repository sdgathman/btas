/** @mainpage BTree Access System
  Copyright 1990,1993,2010 Business Management Systems, Inc
 
  <h1>Low Level Operations</h1>
 
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
  disallowed characters other than <code>NUL</code>.  The low level
  <code>BTOPEN</code> begins at the current directory record, if any,
  so that binary pathnames can be followed by explicitly reading
  directory records and specifying a zero length path for each
  path component.
 
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
  If the key length is 0, then it matches all records in the file.
  For read operations depending on ordering of the records, records are
  ordered by an unsigned byte comparison.  When one record is a prefix of 
  another, the shorter record comes first.  ("A" < "AA" but "AA" < "B".)
  There can never be any duplicate records because of the constraints on
  <code>BTWRITE</code> and <code>BTREPLACE</code>.
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
  <li> <code>BTDELETE</code> fails with <code>BTERDUP</code> if there is more
  than one matching record and fails with <code>BTERKEY</code> if there
  is no matching record.  Otherwise it deletes the matching record.  
  Record length is unused.
  <li> <code>BTDELETEF</code> and <code>BTDELETEL</code> fail with 
  <code>BTERKEY</code> if the set is empty, otherwise they delete the first
  or last record of the set respectively.
  Record length is unused.
  <li> <code>BTREPLACE</code> fails with <code>BTERKEY</code> if the set is
  empty and fails with <code>BTERDUP</code> if the set has more than one
  record.  Otherwise it replaces the matching record with the record provided.
  </ul>

  Each record is in theory of arbitrary length.  <code>BTREREAD</code>
  continues reading the last record read, <code>BTSEEK</code> sets the
  byte offset within the last record for the next BTREREAD, and
  <code>BTREWRITE</code> writes over bytes.  FIXME: this is not yet
  implemented, but BTREWRITE can be used to overwrite the first record length
  bytes of an existing record (which must be unique for key length bytes).

  Directory records are the same as file records, except that they include
  the root node (similar to a unix inode) number of another file or
  directory.  When following a NUL separated pathname, the btserve
  process assumes each directory record begins with the null terminated
  pathname component.  The standard library always adds the null terminator
  to directory records, even when the record has no following data.  This
  prevents surprising results such attempting to delete a file named "A" 
  when a file named "AA" exists and getting BTERDUP.
  The included root node may be zero, in which case the record
  is treated as a symbolic link, with the target path in high level
  text form following the NUL terminated name.  Btserve does not
  itself follow symbolic links, the <code>BTOPEN</code> operation
  returns BTERLINK with the corresponding directory record 
  when it encounters a zero root node.  It is therefore
  possible for an alternate library to interpret symlinks differently.
 
 */
