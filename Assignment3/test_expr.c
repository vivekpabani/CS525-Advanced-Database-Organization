#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"

// helper macros
#define OP_TRUE(left, right, op, message)		\
  do {							\
    Value *result;					\
    MAKE_VALUE(result, DT_INT, -1);			\
    op(left, right, result);				\
    bool b = result->v.boolV;				\
    free(result);					\
    ASSERT_TRUE(b,message);				\
   } while (0)

#define OP_FALSE(left, right, op, message)		\
  do {							\
    Value *result;					\
    MAKE_VALUE(result, DT_INT, -1);			\
    op(left, right, result);				\
    bool b = result->v.boolV;				\
    free(result);					\
    ASSERT_TRUE(!b,message);				\
   } while (0)

// test methods
static void testValueSerialize (void);
static void testOperators (void);
static void testExpressions (void);

char *testName;

// main method
int 
main (void) 
{
  testName = "";

  testValueSerialize();
  testOperators();
  testExpressions();

  return 0;
}

// ************************************************************ 
void
testValueSerialize (void)
{
  testName = "test value serialization and deserialization";

  Value *str_result;
  char *serial_str;

  str_result = stringToValue("i10");
  serial_str = serializeValue(str_result);
  ASSERT_EQUALS_STRING(serial_str, "10", "create Value 10");
  freeVal(str_result);
  free(serial_str);

  str_result = stringToValue("f5.3");
  serial_str = serializeValue(str_result);
  ASSERT_EQUALS_STRING(serial_str, "5.300000", "create Value 5.3");
  freeVal(str_result);
  free(serial_str);

  str_result = stringToValue("sHello World");
  serial_str = serializeValue(str_result);
  ASSERT_EQUALS_STRING(serial_str, "Hello World", "create Value Hello World");
  freeVal(str_result);
  free(serial_str);

  str_result = stringToValue("bt");
  serial_str = serializeValue(str_result);
  ASSERT_EQUALS_STRING(serial_str, "true", "create Value true");
  freeVal(str_result);
  free(serial_str);

  str_result = stringToValue("btrue");
  serial_str = serializeValue(str_result);
  ASSERT_EQUALS_STRING(serial_str, "true", "create Value true");
  freeVal(str_result);
  free(serial_str);

  TEST_DONE();
}

// ************************************************************ 
void
testOperators (void)
{
  Value *result;
  testName = "test value comparison and boolean operators";
  MAKE_VALUE(result, DT_INT, 0);
  
  // equality
  Value *str_result1, *str_result2;
  
  str_result1 = stringToValue("i10");
  str_result2 = stringToValue("i10");
  OP_TRUE(str_result1,str_result2, valueEquals, "10 = 10");
  freeVal(str_result1);
  freeVal(str_result2);

  str_result1 = stringToValue("i9");
  str_result2 = stringToValue("i10");
  OP_FALSE(str_result1,str_result2, valueEquals, "9 != 10");
  freeVal(str_result1);
  freeVal(str_result2);

  str_result1 = stringToValue("sHello World");
  str_result2 = stringToValue("sHello World");
  OP_TRUE(str_result1, str_result2, valueEquals, "Hello World = Hello World");
  freeVal(str_result1);
  freeVal(str_result2);

  str_result1 = stringToValue("sHello Worl");
  str_result2 = stringToValue("sHello World");
  OP_FALSE(str_result1, str_result2, valueEquals, "Hello Worl != Hello World");
  freeVal(str_result1);
  freeVal(str_result2);

  str_result1 = stringToValue("sHello Worl");
  str_result2 = stringToValue("sHello Wor");
  OP_FALSE(str_result1, str_result2, valueEquals, "Hello Worl != Hello Wor");
  freeVal(str_result1);
  freeVal(str_result2);

  // smaller
  str_result1 = stringToValue("i3");
  str_result2 = stringToValue("i10");
  OP_TRUE(str_result1, str_result2, valueSmaller, "3 < 10");
  freeVal(str_result1);
  freeVal(str_result2);
  
  str_result1 = stringToValue("f5.0");
  str_result2 = stringToValue("f6.5");
  OP_TRUE(str_result1, str_result2, valueSmaller, "5.0 < 6.5");
  freeVal(str_result1);
  freeVal(str_result2);

  // boolean

  str_result1 = stringToValue("bt");
  str_result2 = stringToValue("bf");

  OP_TRUE(str_result1, str_result1, boolAnd, "t AND t = t");
  OP_FALSE(str_result1,str_result2, boolAnd, "t AND f = f");
  OP_TRUE(str_result1,str_result2, boolOr, "t OR f = t");
  OP_FALSE(str_result2,str_result2, boolOr, "f OR f = f");

  TEST_CHECK(boolNot(str_result2, result));
  ASSERT_TRUE(result->v.boolV, "!f = t");

  freeVal(str_result1);
  freeVal(str_result2);
  freeVal(result);
  TEST_DONE();
}

// ************************************************************
void
testExpressions (void)
{
  Expr *op, *l, *r;
  Value *res;
  testName = "test complex expressions";
  
  MAKE_CONS(l, stringToValue("i10"));
  evalExpr(NULL, NULL, l, &res);
  OP_TRUE(stringToValue("i10"), res, valueEquals, "Const 10");
  freeVal(res);

  MAKE_CONS(r, stringToValue("i20"));
  evalExpr(NULL, NULL, r, &res);
  OP_TRUE(stringToValue("i20"), res, valueEquals, "Const 20");
  freeVal(res);

  MAKE_BINOP_EXPR(op, l, r, OP_COMP_SMALLER);
  evalExpr(NULL, NULL, op, &res);
  OP_TRUE(stringToValue("bt"), res, valueEquals, "Const 10 < Const 20");
  freeVal(res);

  MAKE_CONS(l, stringToValue("bt"));
  evalExpr(NULL, NULL, l, &res);
  OP_TRUE(stringToValue("bt"), res, valueEquals, "Const true");
  freeVal(res);

  r = op;
  MAKE_BINOP_EXPR(op, r, l, OP_BOOL_AND);
  evalExpr(NULL, NULL, op, &res);
  OP_TRUE(stringToValue("bt"), res, valueEquals, "(Const 10 < Const 20) AND true");
  freeVal(res);
  freeExpr(op); //added
  TEST_DONE();
}
