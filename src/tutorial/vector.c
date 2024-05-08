#include"postgres.h"
#include"fmgr.h"
#include "common/shortest_dec.h"
#include <string.h>
#include <math.h>

PG_MODULE_MAGIC;


typedef struct{
    char vl_len[4];
    int size;
    float a[FLEXIBLE_ARRAY_MEMBER];
}Vector;

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(vector_in);

Datum
vector_in(PG_FUNCTION_ARGS)
{
    char *str= PG_GETARG_CSTRING(0);
    Vector *result;
    int w = strlen(str)-1;
    char *buf = (char*)palloc(w+2);
    char *s;
    char *endptr;
    float *x = (float*)palloc((w+1)*sizeof(float));
    int z=0;
    strcpy(buf,str);
    str[0] = ' ';
    str[w] = '\0';
    for(int i=0;i<=w;i++)
    {
        if(i>=1&&i<=w-1){
            if(buf[i]==','){
                if(!(buf[i-1]<='9'&&buf[i-1]>='0')){
                    ereport(ERROR,
                            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                    errmsg("invalid input syntax for type %s: \"%s\"",
                                           "complex", buf)));}
            }
            else if(buf[i]=='.'){
                if(!(buf[i-1]<='9'&&buf[i-1]>='0')){ereport(ERROR,
                                                            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                                                    errmsg("invalid input syntax for type %s: \"%s\"",
                                                                           "complex", buf)));}
                else if(!(buf[i+1]<='9'&&buf[i+1]>='0')){ereport(ERROR,
                                                                 (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                                                         errmsg("invalid input syntax for type %s: \"%s\"",
                                                                                "complex", buf)));}
            }
        }
        else if(i==0){
            if(buf[i]!='{'){ereport(ERROR,
                                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                            errmsg("invalid input syntax for type %s: \"%s\"",
                                                   "complex", buf)));}
        }
        else{
            if(buf[i]!='}'){ereport(ERROR,
                                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                            errmsg("invalid input syntax for type %s: \"%s\"",
                                                   "complex", buf)));}
            else if(!(buf[i-1]<='9'&&buf[i-1]>='0')){ereport(ERROR,
                                                            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                                                    errmsg("invalid input syntax for type %s: \"%s\"",
                                                                           "complex", buf)));}

        }
    }
    s = strtok(str,",");
    for(;s!=NULL;s=strtok(NULL,",")){
        x[z++] = strtof(s,&endptr);
        if(*endptr!='\0'){ereport(ERROR,
                                  (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                                          errmsg("invalid input syntax for type %s: \"%s\"",
                                                 "complex", buf)));}
    }
    if(z==0){ereport(ERROR,
                     (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                             errmsg("invalid input syntax for type %s: \"%s\"",
                                    "complex", buf)));}
    result = (Vector *) palloc(VARHDRSZ+sizeof(int)+sizeof(float)*z);
    SET_VARSIZE(result,VARHDRSZ+sizeof(int)+sizeof(float)*z);
    result->size = z;
    for(int j=0;j<z;j++){
        result->a[j] = (float)x[j];
    }
    PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(vector_out);

Datum
vector_out(PG_FUNCTION_ARGS)
{
    Vector *vector = (Vector *) PG_GETARG_POINTER(0);
    char *result = (char*)palloc(2+vector->size);
    int index=1;
    char   *result1 = (char*)palloc(1024*sizeof(float));
    result[0] = '{';
    for(int i=0;i<vector->size;i++){
        float_to_shortest_decimal_buf(vector->a[i],result1);
        for(int j=0;j<(int)strlen(result1);j++){
            result[index++] = (char)result1[j];
        }
        if(i!=vector->size-1){result[index++]=',';}
        else{result[index++] = '}';}
    }
    result[index] = '\0';
    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
* New Operators
*
* A practical Vector datatype would provide much more than this, of course.
*****************************************************************************/


PG_FUNCTION_INFO_V1(vector_getsize);

Datum
vector_getsize(PG_FUNCTION_ARGS)
{
    Vector * vector = (Vector*) PG_GETARG_POINTER(0);

    PG_RETURN_INT32(vector->size);
}

PG_FUNCTION_INFO_V1(vector_get_distance);

Datum
vector_get_distance(PG_FUNCTION_ARGS)
{
    Vector * a = (Vector*) PG_GETARG_POINTER(0);
    Vector * b = (Vector*) PG_GETARG_POINTER(1);
    float result = 0;

    if(a->size!=b->size){
        ereport(ERROR,
                errmsg("operator is not unique : get vector%d to vector%d L2distance",a->size,b->size));
    }
    for(int i=0;i<a->size;i++){
        result += (a->a[i]-b->a[i])*(a->a[i]-b->a[i]);
    }
    result = sqrt(result);
    PG_RETURN_FLOAT4(result);
}

PG_FUNCTION_INFO_V1(vector_sub);

Datum
vector_sub(PG_FUNCTION_ARGS)
{
    Vector * a = (Vector*) PG_GETARG_POINTER(0);
    Vector * b = (Vector*) PG_GETARG_POINTER(1);
    Vector * result;

    if(a->size!=b->size){
        ereport(ERROR,
                errmsg("operator is not unique : vector%d - vector%d",a->size,b->size));}

    result = (Vector *) palloc(VARHDRSZ+sizeof(int)+sizeof(float)*a->size);
    SET_VARSIZE(result,VARHDRSZ+sizeof(int)+sizeof(float)*a->size);
    result->size = a->size;
    for(int i=0;i<a->size;i++)
    {
        result->a[i] = (float)(a->a[i]-b->a[i]);
    }
    PG_RETURN_POINTER(result);


}

PG_FUNCTION_INFO_V1(vector_add);

Datum
vector_add(PG_FUNCTION_ARGS)
{
    Vector * a = (Vector*) PG_GETARG_POINTER(0);
    Vector * b = (Vector*) PG_GETARG_POINTER(1);
    Vector * result;

    if(a->size!=b->size){
        ereport(ERROR,
                errmsg("operator is not unique : vector%d + vector%d",a->size,b->size));}

    result = (Vector *) palloc(VARHDRSZ+sizeof(int)+sizeof(float)*a->size);
    SET_VARSIZE(result,VARHDRSZ+sizeof(int)+sizeof(float)*a->size);
    result->size = a->size;
    for(int i=0;i<a->size;i++)
    {
        result->a[i] = (float)(a->a[i]+b->a[i]);
    }
    PG_RETURN_POINTER(result);
}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
