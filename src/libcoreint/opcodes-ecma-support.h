/* Copyright 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPCODES_ECMA_SUPPORT_H
#define OPCODES_ECMA_SUPPORT_H

#include "ecma-alloc.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"
#include "ecma-number-arithmetic.h"
#include "ecma-operations.h"
#include "ecma-try-catch-macro.h"
#include "ecma-objects.h"

#include "opcodes.h"

static bool do_strict_eval_arguments_check (ecma_reference_t) __unused;
static ecma_completion_value_t get_variable_value (__int_data *, idx_t, bool) __unused;
static ecma_completion_value_t set_variable_value (__int_data *, idx_t, ecma_value_t) __unused;

/**
 * Perform so-called 'strict eval or arguments reference' check
 * that is used in definition of several statement handling algorithms,
 * but has no ECMA-defined name.
 *
 * @return true - if ref is strict reference
 *                   and it's base is lexical environment
 *                   and it's referenced name is 'eval' or 'arguments';
 *         false - otherwise.
 */
static bool
do_strict_eval_arguments_check (ecma_reference_t ref) /**< ECMA-reference */
{
  bool ret;

  if (ref.is_strict
      && (ref.base.value_type == ECMA_TYPE_OBJECT)
      && (ECMA_GET_POINTER (ref.base.value) != NULL)
      && (((ecma_object_t*) ECMA_GET_POINTER (ref.base.value))->is_lexical_environment))
  {
    ecma_string_t* magic_string_eval = ecma_get_magic_string (ECMA_MAGIC_STRING_EVAL);
    ecma_string_t* magic_string_arguments = ecma_get_magic_string (ECMA_MAGIC_STRING_ARGUMENTS);

    ret = (ecma_compare_ecma_string_to_ecma_string (ref.referenced_name_p,
                                                    magic_string_eval) == 0
           || ecma_compare_ecma_string_to_ecma_string (ref.referenced_name_p,
                                                       magic_string_arguments) == 0);

    ecma_deref_ecma_string (magic_string_eval);
    ecma_deref_ecma_string (magic_string_arguments);
  }
  else
  {
    ret = false;
  }

  return ret;
} /* do_strict_eval_arguments_check */

/**
 * Get variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
get_variable_value (__int_data *int_data, /**< interpreter context */
                    idx_t var_idx, /**< variable identifier */
                    bool do_eval_or_arguments_check) /** run 'strict eval or arguments reference' check
                                                        See also: do_strict_eval_arguments_check */
{
  ecma_completion_value_t ret_value;

  if (var_idx >= int_data->min_reg_num
      && var_idx <= int_data->max_reg_num)
  {
    ecma_value_t reg_value = int_data->regs_p[ var_idx - int_data->min_reg_num ];

    JERRY_ASSERT (!ecma_is_value_empty (reg_value));

    ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                            ecma_copy_value (reg_value, true),
                                            ECMA_TARGET_ID_RESERVED);
  }
  else
  {
    ecma_reference_t ref;

    ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (var_idx);

    ref = ecma_op_get_identifier_reference (int_data->lex_env_p,
                                            var_name_string_p,
                                            int_data->is_strict);

    if (unlikely (do_eval_or_arguments_check
                  && do_strict_eval_arguments_check (ref)))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
    }
    else
    {
      ret_value = ecma_op_get_value (ref);
    }

    ecma_deref_ecma_string (var_name_string_p);
    ecma_free_reference (ref);
  }

  return ret_value;
} /* get_variable_value */

/**
 * Set variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
set_variable_value (__int_data *int_data, /**< interpreter context */
                    idx_t var_idx, /**< variable identifier */
                    ecma_value_t value) /**< value to set */
{
  ecma_completion_value_t ret_value;

  if (var_idx >= int_data->min_reg_num
      && var_idx <= int_data->max_reg_num)
  {
    ecma_value_t reg_value = int_data->regs_p[ var_idx - int_data->min_reg_num ];

    if (!ecma_is_value_empty (reg_value))
    {
      ecma_free_value (reg_value, true);
    }

    int_data->regs_p[ var_idx - int_data->min_reg_num ] = ecma_copy_value (value, true);

    ret_value = ecma_make_empty_completion_value ();
  }
  else
  {
    ecma_reference_t ref;

    ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (var_idx);

    ref = ecma_op_get_identifier_reference (int_data->lex_env_p,
                                            var_name_string_p,
                                            int_data->is_strict);

    if (unlikely (do_strict_eval_arguments_check (ref)))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
    }
    else
    {
      ret_value = ecma_op_put_value (ref, value);
    }

    ecma_deref_ecma_string (var_name_string_p);
    ecma_free_reference (ref);
  }

  return ret_value;
} /* set_variable_value */

#endif /* OPCODES_ECMA_SUPPORT_H */