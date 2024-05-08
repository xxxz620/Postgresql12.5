---------------------------------------------------------------------------
--
-- vector.sql-
--    This file shows how to create a new user-defined type and how to
--    use this new type.
--
--
-- Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
-- Portions Copyright (c) 1994, Regents of the University of California
--
-- src/tutorial/vector.source
--
---------------------------------------------------------------------------

-----------------------------
-- Creating a new type:
--	We are going to create a new type called 'vector' which represents
--	vector numbers.
--	A user-defined type must have an input and an output function, and
--	optionally can have binary input and output functions.  All of these
--	are usually user-defined C functions.
-----------------------------

-- Assume the user defined functions are in /home/xtz/postgresql-12.5/src/tutorial/vector$DLSUFFIX
-- (we do not want to assume this is in the dynamic loader search path).
-- Look at $PWD/complex.c for the source.  Note that we declare all of
-- them as STRICT, so we do not need to cope with NULL inputs in the
-- C code.  We also mark them IMMUTABLE, since they always return the
-- same outputs given the same inputs.

-- the input function 'vector_in' takes a null-terminated string (the
-- textual representation of the type) and turns it into the internal
-- (in memory) representation. You will get a message telling you 'vector'
-- does not exist yet but that's okay.

CREATE FUNCTION vector_in(cstring)
   RETURNS vector
   AS '/home/xtz/postgresql-12.5/src/tutorial/vector'
   LANGUAGE C IMMUTABLE STRICT;

-- the output function 'complex_out' takes the internal representation and
-- converts it into the textual representation.

CREATE FUNCTION vector_out(vector)
   RETURNS cstring
   AS '/home/xtz/postgresql-12.5/src/tutorial/vector'
   LANGUAGE C IMMUTABLE STRICT;


-- now, we can create the type. The internallength specifies the size of the
-- memory block required to hold the type (we need two 8-byte doubles).

CREATE TYPE vector (
   input = vector_in,
   output = vector_out
);

-----------------------------
-- Creating an operator for the new type:
--	Let's define an add operator for complex types. Since POSTGRES
--	supports function overloading, we'll use + as the add operator.
--	(Operator names can be reused with different numbers and types of
--	arguments.)
-----------------------------

-- first, define a function vector_add (also in vector.c)
CREATE FUNCTION vector_add(vector, vector)
   RETURNS vector
   AS '/home/xtz/postgresql-12.5/src/tutorial/vector'
   LANGUAGE C IMMUTABLE STRICT;

-- we can now define the operator. We show a binary operator here but you
-- can also define unary operators by omitting either of leftarg or rightarg.
CREATE OPERATOR + (
   leftarg = vector,
   rightarg = vector,
   procedure = vector_add,
   commutator = +
);

-- first, define a function vector_sub (also in vector.c)
CREATE FUNCTION vector_sub(vector, vector)
   RETURNS vector
   AS '/home/xtz/postgresql-12.5/src/tutorial/vector'
   LANGUAGE C IMMUTABLE STRICT;

-- we can now define the operator. We show a binary operator here but you
-- can also define unary operators by omitting either of leftarg or rightarg.
CREATE OPERATOR - (
   leftarg = vector,
   rightarg = vector,
   procedure = vector_sub,
   commutator = -
);

-- first, define a function vector_getsize(also in vector.c)
CREATE FUNCTION vector_getsize(vector)
   RETURNS integer
   AS '/home/xtz/postgresql-12.5/src/tutorial/vector'
   LANGUAGE C IMMUTABLE STRICT;


CREATE OPERATOR <#> (
   rightarg = vector,
   procedure = vector_getsize
);

-- first, define a function vector_get_distance(also in vector.c)
CREATE FUNCTION vector_get_distance(vector,vector)
  RETURNS float4
  AS '/home/xtz/postgresql-12.5/src/tutorial/vector'
  LANGUAGE C IMMUTABLE STRICT;


CREATE OPERATOR <-> (
  leftarg = vector,
  rightarg = vector,
  procedure = vector_get_distance,
  commutator = <->
  );

create table test_vector (id integer primary key, vec vector);
insert into test_vector values (1, '{-1.2, 2.1345, 3}');
insert into test_vector values (2, '{-1.2 , 2.1345, 3}');
insert into test_vector values (3, ' {-1.2, 2.1345, 3}');
insert into test_vector values (4, '{-1. 0, 2,3}');
select <#>'{1,2}';
select '{1,2}'::vector<->'{3,4}'::vector;
select *, '{1,2,3}'<->vec as dis from test_vector order by dis  limit 10;
select '{1.1,2.2}'::vector-'{3.3,4.4}'::vector;
select '{1.1,2.2}'::vector+'{3.3,4.4}'::vector;


