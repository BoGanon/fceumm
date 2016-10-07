#include <libmtap.h>
#include <input.h>

#include "../../types.h"
#include "../../fceu.h"
#include "driver.h"

static pad_t *pads[8];
static int pad_num = 0;
int nes_pads;
unsigned int old_data[8];
unsigned int turbo_toggle_a[8];
unsigned int turbo_toggle_b[8];
unsigned int exit_game = 0;

char multitap_ports[8] =
{
	0, 0, 0, 0,
	1, 1, 1, 1
};

char multitap_slots[8] =
{
	0, 1, 2, 3,
	0, 1, 2, 3
};

void ps2_input_open_pads(void)
{

	int i = 0;

	pad_t *pad;

	// hardcoded controllers
	for (i = 0; i < 8; i++)
	{

		pad = pad_open(multitap_ports[i],multitap_slots[i],MODE_DIGITAL,MODE_UNLOCKED);

		if (pad != NULL)
		{
			printf("Pad(%d,%d) initialized.\n",multitap_ports[i],multitap_slots[i]);

			pads[pad_num] = pad;
			pad_num++;
		}
	}

}

void ps2_input_close_pads(void)
{
	int i;

	for (i = 0; i < pad_num; i++)
	{
		pad_close(pads[i]);
	}

	pad_num = 0;

}

void ps2_input_nes_init(void)
{
	int attrib = 0;

	FCEUI_DisableFourScore(1);

	FCEUI_SetInput(0, SI_GAMEPAD, (char*)&nes_pads, attrib);
	FCEUI_SetInput(1, SI_GAMEPAD, (char*)&nes_pads, attrib);

}

static inline unsigned char ps2_report_button(int nes, int pressed)
{
	return pressed ? nes : 0;
}

static void FCEUI_Exit()
{
	exit_game = 1;
}

static inline unsigned char ps2_get_ps2_input(const int controller)
{
	static int stop = 0;
	unsigned char P = 0;
	int ret = 0;
	unsigned int data;
	//unsigned int new_data;
	pad_t *cur_pad;

	if (!pads[controller])
	{
		return 0;
	}

	cur_pad = pads[controller];

	//check to see if pads are disconnected
	pad_get_state(cur_pad);
	while((cur_pad->state != PAD_STATE_STABLE) && (cur_pad->state != PAD_STATE_FINDCTP1))
	{
		if(cur_pad->state==PAD_STATE_DISCONN)
		{
			printf("Pad(%d, %d) is disconnected\n", cur_pad->port, cur_pad->slot);
			break;
		}
		pad_get_state(cur_pad);
	}

	ret = padRead(cur_pad->port,cur_pad->slot,cur_pad->buttons);
	if (ret != 0)
	{
		data = 0xffff ^ cur_pad->buttons->btns;
		//new_data = data & ~old_data[controller];
		old_data[controller] = data;

		if (!controller)
		{
			if(data != PAD_START)
			{
				stop = 0;
			}
			else if (data == PAD_START)
			{
				stop++;
				if (stop == 100)
				{
					stop = 0;
					FCEUI_Exit();
				}
			}
		}

		if (data & PAD_R2)
		{
			FCEUI_SaveState(NULL);
		}
		if (data & PAD_L2)
		{
			FCEUI_LoadState(NULL);
		}
		//JOY_Button is NES
		P |= ps2_report_button(JOY_A,data & PAD_CROSS);
		P |= ps2_report_button(JOY_B,data & PAD_SQUARE);
		P |= ps2_report_button(JOY_SELECT,data & PAD_SELECT);
		P |= ps2_report_button(JOY_START,data & PAD_START);
		P |= ps2_report_button(JOY_UP,data & PAD_UP);
		P |= ps2_report_button(JOY_DOWN,data & PAD_DOWN);
		P |= ps2_report_button(JOY_LEFT,data & PAD_LEFT);
		P |= ps2_report_button(JOY_RIGHT,data & PAD_RIGHT);
/*
		if(data & PAD_CROSS)
		{
			if(turbo_toggle_a[controller])
			{
				rapid_a ^= 1;
			}
			else
			{
				P |= JOY_A;
			}
		}

		if(data & PAD_CIRCLE))
		{
			turbo_toggle_a[controller] ^= 1;
		}

		if(data & PAD_SQUARE)
		{
			if(rapidfire_b[port])
			{
				rapid_b[port] ^= 1;
			}
			else
			{
				P |= JOY_B;
			}
        }

		if(data & PAD_TRIANGLE))
        {
			turbo_toggle_b[controller] ^=1;
		}

		if(data & PAD_SELECT)
		{
			P |= JOY_SELECT;
		}
		if(data & PAD_START)
		{
			P |= JOY_START;
		}

		//Analog
		if ((buttons[port].mode >> 4) == 0x07)
		{
			if (buttons[port].ljoy_h < 64)
				P |= JOY_LEFT;
			else if (buttons[port].ljoy_h > 192)
				P |= JOY_RIGHT;
			if (buttons[port].ljoy_v < 64)
				P |= JOY_UP;
			else if (buttons[port].ljoy_v > 192)
				P |= JOY_DOWN;
		}

		//Digital
		if(data & PAD_UP)
		{
			P |= JOY_UP;
		}
		if(data & PAD_DOWN)
		{
			P |= JOY_DOWN;
		}
		if(data & PAD_LEFT)
		{
			P |= JOY_LEFT;
		}
		if(data & PAD_RIGHT)
		{
			P |= JOY_RIGHT;
        }
*/
    }

    return P;

}

int ps2_get_nes_input(void)
{

	if(exit_game)
	{
		nes_pads = 0;
		exit_game = 0;
		return 1;
	}
/*
	if(Settings.turbo)
	{
		nes_pads = ps2_get_ps2_input(0);
		nes_pads |= ps2_get_ps2_input(1) << 8;

		if(turbo_toggle_a[0])
			nes_pads |= JOY_A;

		if(turbo_toggle_a[0])
			nes_pads |= JOY_B;

		if(turbo_toggle_a[1])
			nes_pads |= 0x100;

		if(turbo_toggle_b[1])
			nes_pads |= 0x200;
	}
	else
*/
	{
        nes_pads = ps2_get_ps2_input(0);
        // Second pad is 8 bits higher
        //nes_pads |= ps2_get_ps2_input(1) << 8;
    }

    return 0;
}

extern "C" char* GetKeyboard(void)
{
	return NULL;
}
