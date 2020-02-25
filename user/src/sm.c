#include "common.h"
#include "sm.h"
#include "SPI.h"
#include "adc_spi.h"

enum ret_codes { ok, fail, repeat};

enum ret_codes state_12345(void);
enum ret_codes state_06(void);
enum ret_codes state_7(void);

//             |    repeat   |    fail     |    ok     |
//-----------------------------------------------------|
//state_12345  | state_12345 |             |  state_06 |
//-----------------------------------------------------|
//state_06     |   state_06  | state_12345 |  state_7  |
//-----------------------------------------------------|
//state_7      |             |             |  state_06 |
//-----------------------------------------------------|

enum ret_codes (* state[])(void) = { state_12345, state_06, state_7};
enum state_codes { sc_12345, sc_06, sc_7, end};


uint16_t (* cmd_callback[])(void) =
{ adc_cmd0_callback, adc_cmd1_callback, adc_cmd2_callback, adc_cmd3_callback, adc_cmd4_callback, adc_cmd5_callback, adc_cmd6_callback};

void (* cmd_data_callback[])(uint16_t) =
{ 0, 0, adc_cmd2_data_callback, adc_cmd3_data_callback, adc_cmd4_data_callback, adc_cmd5_data_callback, 0};


extern uint8_t Cmd_SPI_In[2];

uint8_t cmd_idx;

extern uint8_t Cmd_SPI_Out[2];

inline void prepare_cmd_out(void)
{
    uint16_t const value = cmd_callback[cmd_idx]();
    Cmd_SPI_Out[0] = value;
    Cmd_SPI_Out[1] = value >> 8;
}
inline void prepare_cmd_empty_out(void)
{
    Cmd_SPI_Out[0] = 0xFF;
    Cmd_SPI_Out[1] = 0xFF;
}

#define SPI_CMD_QUERY_MACRO(CHECK_CMD_FUNC)         \
    if (ss == query_stage)                          \
    {                                               \
       SPI_CMD_QUERY();                             \
       cmd_idx = Cmd_SPI_In[0];                     \
       if (CHECK_CMD_FUNC())                        \
       {                                            \
          prepare_cmd_out();                        \
       } else                                       \
       {                                            \
          prepare_cmd_empty_out();                  \
       }                                            \
       ss = reply_stage;                            \
       return repeat;                               \
    }

#define SPI_CMD_REPLY_MACRO(RET_FUNC)                                       \
    ss = query_stage;                                                       \
    SPI_CMD_REPLY();                                                        \
    if (cmd_data_callback[cmd_idx])                                         \
    {                                                                       \
       cmd_data_callback[cmd_idx]( (Cmd_SPI_In[1] << 8) | Cmd_SPI_In[0] );  \
    }                                                                       \
    return RET_FUNC();

#define STOP_ADC_COMMAND 0
#define START_ADC_COMMAND 1
#define SETUP_CHANNEL_COUNT_COMMAND 2
#define SETUP_INPUT_RANGE_CODE_COMMAND 3
#define SETUP_SAMPLE_SIZE_COMMAND 4
#define SETUP_CHANNEL_RATE_CODE_COMMAND 5
#define CHECK_DATA_READY_COMMAND 6
		
inline uint8_t check_cmd_12345()
{
    return ((cmd_idx < START_ADC_COMMAND) || (cmd_idx > SETUP_CHANNEL_RATE_CODE_COMMAND)) ? 0 : 1;
}

inline uint8_t check_cmd_06()
{
    return ((cmd_idx == CHECK_DATA_READY_COMMAND) || (cmd_idx == STOP_ADC_COMMAND)) ? 1 : 0;
}

inline enum ret_codes ret_func_12345(void)
{
    return (cmd_idx == START_ADC_COMMAND) ? ok : repeat;
}

extern uint8_t data_ready_flag;
inline enum ret_codes ret_func_06(void)
{
    if (cmd_idx == CHECK_DATA_READY_COMMAND)
    {
        if (data_ready_flag)
        {
            return ok;
        } else
        {
            return repeat;
        }
    }
    return fail;
}


enum state_stage {query_stage, reply_stage};
enum state_stage ss = query_stage;

enum ret_codes state_12345(void)
{
    SPI_CMD_QUERY_MACRO(check_cmd_12345)
    SPI_CMD_REPLY_MACRO(ret_func_12345)
}

enum ret_codes state_06(void)
{
    SPI_CMD_QUERY_MACRO(check_cmd_06)
    SPI_CMD_REPLY_MACRO(ret_func_06)
}

extern __IO uint8_t data_ready;
enum ret_codes state_7(void)
{
    data_ready = 0;

    GPIO_ResetBits (GPIOD, GPIO_Pin_15);

    SPI_ARRAY();
    return ok; // state_06
}

struct transition
{
    enum state_codes src_state;
    enum ret_codes   ret_code;
    enum state_codes dst_state;
};
/* transitions from end state aren't needed */
struct transition state_transitions[] =
{
    {sc_06,    repeat, sc_06},
    {sc_06,    ok,     sc_7},
    {sc_7,     ok,     sc_06},
    {sc_12345, repeat, sc_12345},
    {sc_12345, ok,     sc_06},
    {sc_06,    fail,   sc_12345}
};


uint8_t max_elements = 0;
enum state_codes fsmError = end;

enum state_codes lookup_transitions (enum state_codes const curr_st, enum ret_codes r_c)
{
    enum state_codes next_state = fsmError;
    for (uint8_t i = 0; i < max_elements; ++i)
    {
        if (((state_transitions[i].src_state) == curr_st) && ((state_transitions[i].ret_code) == r_c))
        {
            next_state = state_transitions[i].dst_state;
            break;
        }
    }
    return next_state;
}

#define ENTRY_STATE sc_12345
enum state_codes cur_state = ENTRY_STATE;

void process(void)
{
    max_elements = sizeof(state_transitions)/sizeof(state_transitions[0]);
    for (;;)
    {
        if ((cur_state = lookup_transitions(cur_state, state[cur_state]())) == fsmError)
        {
            return;
        }
    }
}

