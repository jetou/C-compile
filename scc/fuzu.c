TRY_PUNCT('token')//看看下一个token是不是'token'是的话跳到下一个token并返回true
EXPECT_PUNCT(')')//如果下一个符号不是')',并且类型不是TK_PUNCT报错
parse_expr()//待定
parse_cast_expr()//待定
parse_assign_expr()//待定
NEXT()//调用lexer的 get_token 函数获取下一个字符
dict_lookup(dict_t *dict, const char *key)//在dict中找到hash为key的值没有则返回Null
node_t //语法分析树结构体
make_number()//创建一个number的node节点
make_char()
make_string()
make_vector()//创建一个默认的vector_t结构，size为8
make_arith_conv(ctype_t *ctype, node_t *expr)//新加一个节点
node_t->ctpy_t->vector_t
is_arith_type(ctype_t *type)//判断type是ctype_int 还是 ctype_float 还是 ctype_double 类型的都不是得话返回false是的话返回true
is_same_type(ctype_t *t, ctype_t *p)//判断t,p是否相等，以及判断t,p是否都是数组或指针，和下一个链是否都是void型又或者下一个链的值是一样的。以上是的话返回true否返回false
is_ptr(ctype_t *ctype)//判断ctpye 是否是指针或是数组是的话返回true不是返回false
is_array()如果type == array 就返回true
conv()//先用is_same_type 判断如果返回的值是false 再用 make_arith_conv()
parse_cond_expr()//检查是二元运算符还是三目运算符


