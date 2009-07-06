/*
 * libtilemcore - Graphing calculator emulation library
 *
 * Copyright (C) 2009 Benjamin Moody
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "tilem.h"
#include "z80.h"

/* Timer manipulation */

/*
static void dumptimers(TilemZ80* z80)
{
	int tmr;
	int t;

	printf("*** RT:");
	for (tmr = z80->timer_rt; tmr; tmr = z80->timers[tmr].next) {
		t = z80->timers[tmr].count - z80->clock;
		printf(" %d:%d", tmr, t);
	}
	printf("\n*** CPU:");
	for (tmr = z80->timer_cpu; tmr; tmr = z80->timers[tmr].next) {
		t = z80->timers[tmr].count - z80->clock;
		printf(" %d:%d", tmr, t);
	}
	printf("\n*** Free:");
	for (tmr = z80->timer_free; tmr; tmr = z80->timers[tmr].next) {
		printf(" %d", tmr);
	}
	printf("\n");
}
*/

static inline void timer_free(TilemZ80* z80, int tmr)
{
	z80->timers[tmr].callback = NULL;
	z80->timers[tmr].callbackdata = NULL;
	z80->timers[tmr].next = z80->timer_free;
	z80->timers[tmr].prev = 0;
	z80->timer_free = tmr;
}

static inline int timer_alloc(TilemZ80* z80)
{
	int tmr, i;

	if (z80->timer_free) {
		tmr = z80->timer_free;
		z80->timer_free = z80->timers[tmr].next;
		z80->timers[tmr].next = 0;
		return tmr;
	}

	i = z80->ntimers;
	z80->ntimers = i * 2 + 1;
	z80->timers = tilem_renew(TilemZ80Timer, z80->timers, z80->ntimers);
	while (i < z80->ntimers) {
		timer_free(z80, i);
		i++;
	}

	tmr = z80->timer_free;
	z80->timer_free = z80->timers[tmr].next;
	z80->timers[tmr].next = 0;
	return tmr;
}

static inline int timer_earlier(TilemZ80* z80, int tmr1, int tmr2)
{
	dword count1, count2;

	count1 = z80->timers[tmr1].count + 10000 - z80->clock;
	count2 = z80->timers[tmr2].count + 10000 - z80->clock;

	return (count1 < count2);
}

static inline void timer_insert(TilemZ80* z80, int* list, int tmr)
{
	int prev, next;

	if (!*list || timer_earlier(z80, tmr, *list)) {
		z80->timers[tmr].prev = 0;
		z80->timers[tmr].next = *list;
		z80->timers[*list].prev = tmr;
		*list = tmr;
		return;
	}

	prev = *list;
	next = z80->timers[prev].next;

	while (next && timer_earlier(z80, next, tmr)) {
		prev = next;
		next = z80->timers[prev].next;
	}

	z80->timers[prev].next = tmr;
	z80->timers[next].prev = tmr;
	z80->timers[tmr].prev = prev;
	z80->timers[tmr].next = next;
}

static inline void timer_set(TilemZ80* z80, int tmr, dword count,
			     dword period, int rt, dword extra)
{
	dword clocks;
	qword kclocks;

	if (!count) {
		/* leave timer disabled */
		z80->timers[tmr].prev = 0;
		z80->timers[tmr].next = 0;
	}
	else if (rt) {
		kclocks = z80->clockspeed * count;
		clocks = (kclocks + 500) / 1000 - extra;
		z80->timers[tmr].count = z80->clock + clocks;
		z80->timers[tmr].period = period;
		timer_insert(z80, &z80->timer_rt, tmr);
	}
	else {
		clocks = count - extra;
		z80->timers[tmr].count = z80->clock + clocks;
		z80->timers[tmr].period = period;
		timer_insert(z80, &z80->timer_cpu, tmr);
	}
}

static inline void timer_unset(TilemZ80* z80, int tmr)
{
	int prev, next;

	if (tmr == z80->timer_cpu)
		z80->timer_cpu = z80->timers[tmr].next;
	if (tmr == z80->timer_rt)
		z80->timer_rt = z80->timers[tmr].next;

	prev = z80->timers[tmr].prev;
	next = z80->timers[tmr].next;
	z80->timers[prev].next = next;
	z80->timers[next].prev = prev;
	z80->timers[tmr].prev = 0;
	z80->timers[tmr].next = 0;
}


/* Breakpoint manipulation */

static inline void bp_free(TilemZ80* z80, int bp)
{
	z80->breakpoints[bp].type = 0;
	z80->breakpoints[bp].testfunc = NULL;
	z80->breakpoints[bp].testdata = NULL;
	z80->breakpoints[bp].next = z80->breakpoint_free;
	z80->breakpoints[bp].prev = 0;
	z80->breakpoint_free = bp;
}

static inline int bp_alloc(TilemZ80* z80)
{
	int bp, i;

	if (z80->breakpoint_free) {
		bp = z80->breakpoint_free;
		z80->breakpoint_free = z80->breakpoints[bp].next;
		return bp;
	}

	i = z80->nbreakpoints;
	z80->nbreakpoints = i * 2 + 2;
	z80->breakpoints = tilem_renew(TilemZ80Breakpoint, z80->breakpoints,
				       z80->nbreakpoints);
	while (i < z80->nbreakpoints) {
		bp_free(z80, i);
		i++;
	}

	bp = z80->breakpoint_free;
	z80->breakpoint_free = z80->breakpoints[bp].next;
	return bp;
}

static void invoke_ptimer(TilemCalc* calc, void* data)
{
	(*calc->hw.z80_ptimer)(calc, TILEM_PTR_TO_DWORD(data));
}

/* Z80 API */

void tilem_z80_reset(TilemCalc* calc)
{
	int i;

	AF = BC = DE = HL = AF2 = BC2 = DE2 = HL2 = 0xffff;
	IX = IY = IR = SP = WZ = WZ2 = 0xffff;
	PC = 0;
	Rh = 0x80;
	IFF1 = IFF2 = IM = 0;
	calc->z80.interrupts = 0;
	calc->z80.clock = 0;
	calc->z80.lastwrite = 0;

	/* Unset existing timers (they aren't freed, merely
	   disabled) */
	while (calc->z80.timer_cpu)
		timer_unset(&calc->z80, calc->z80.timer_cpu);
	while (calc->z80.timer_rt)
		timer_unset(&calc->z80, calc->z80.timer_rt);

	/* Set up hardware timers */
	if (!calc->z80.ntimers) {
		calc->z80.ntimers = (calc->hw.nhwtimers
				+ TILEM_NUM_SYS_TIMERS + 1);
		tilem_free(calc->z80.timers);
		calc->z80.timers = tilem_new(TilemZ80Timer, calc->z80.ntimers);

		for (i = 1; i < calc->z80.ntimers; i++) {
			calc->z80.timers[i].next = 0;
			calc->z80.timers[i].prev = 0;
			calc->z80.timers[i].count = 0;
			calc->z80.timers[i].period = 0;
			calc->z80.timers[i].callback = &invoke_ptimer;
			calc->z80.timers[i].callbackdata = TILEM_DWORD_TO_PTR(i);
		}

		calc->z80.timers[TILEM_TIMER_LCD_DELAY].callback
			= &tilem_lcd_delay_timer;
		calc->z80.timers[TILEM_TIMER_FLASH_DELAY].callback
			= tilem_flash_delay_timer;
		calc->z80.timers[TILEM_TIMER_LINK_ASSIST].callback
			= tilem_linkport_assist_timer;
	}
}

void tilem_z80_stop(TilemCalc* calc, dword reason)
{
	if (!(reason & calc->z80.stop_mask)) {
		calc->z80.stop_reason |= reason;
		calc->z80.stopping = 1;
	}
}

void tilem_z80_set_speed(TilemCalc* calc, int speed)
{
	int tmr;
	qword t;
	int oldspeed = calc->z80.clockspeed;

	if (oldspeed == speed)
		return;

	for (tmr = calc->z80.timer_rt; tmr; tmr = calc->z80.timers[tmr].next) {
		if ((calc->z80.clock - calc->z80.timers[tmr].count) < 10000)
			continue;

		t = calc->z80.timers[tmr].count - calc->z80.clock;
		t = (t * speed + oldspeed / 2) / oldspeed;
		calc->z80.timers[tmr].count = calc->z80.clock + t;
	}

	calc->z80.clockspeed = speed;
}

int tilem_z80_add_timer(TilemCalc* calc, dword count, dword period,
			int rt, TilemZ80TimerFunc func, void* data)
{
	int id;

	id = timer_alloc(&calc->z80);
	calc->z80.timers[id].callback = func;
	calc->z80.timers[id].callbackdata = data;
	timer_set(&calc->z80, id, count, period, rt, 0);
	return id;
}

void tilem_z80_set_timer(TilemCalc* calc, int id, dword count,
			 dword period, int rt)
{
	if (id < 1 || id > calc->z80.ntimers
	    || !calc->z80.timers[id].callback) {
		tilem_internal(calc, "setting invalid timer %d", id);
		return;
	}
	timer_unset(&calc->z80, id);
	timer_set(&calc->z80, id, count, period, rt, 0);
}

void tilem_z80_set_timer_period(TilemCalc* calc, int id, dword period)
{
	if (id < 1 || id > calc->z80.ntimers
	    || !calc->z80.timers[id].callback) {
		tilem_internal(calc, "setting invalid timer %d", id);
		return;
	}

	calc->z80.timers[id].period = period;
}

void tilem_z80_remove_timer(TilemCalc* calc, int id)
{
	if (id <= calc->hw.nhwtimers + TILEM_NUM_SYS_TIMERS
	    || id > calc->z80.ntimers || !calc->z80.timers[id].callback) {
		tilem_internal(calc, "removing invalid timer %d", id);
		return;
	}
	timer_unset(&calc->z80, id);
	timer_free(&calc->z80, id);
}

int tilem_z80_get_timer_clocks(TilemCalc* calc, int id)
{
	if (id < 1 || id > calc->z80.ntimers
	    || !calc->z80.timers[id].callback) {
		tilem_internal(calc, "querying invalid timer %d", id);
		return 0;
	}
	return (calc->z80.timers[id].count - calc->z80.clock);
}

int tilem_z80_get_timer_microseconds(TilemCalc* calc, int id)
{
	int n = tilem_z80_get_timer_clocks(calc, id);

	if (n < 0) {
		n = ((((qword) -n * 1000) + (calc->z80.clockspeed / 2))
		     / calc->z80.clockspeed);
		return -n;
	}
	else {
		n = ((((qword) n * 1000) + (calc->z80.clockspeed / 2))
		     / calc->z80.clockspeed);
		return n;
	}
}

int tilem_z80_add_breakpoint(TilemCalc* calc, int type,
			     dword start, dword end, dword mask,
			     TilemZ80BreakpointFunc func,
			     void* data)
{
	int bp;

	bp = bp_alloc(&calc->z80);

	calc->z80.breakpoints[bp].type = type;
	calc->z80.breakpoints[bp].start = start;
	calc->z80.breakpoints[bp].end = end;
	calc->z80.breakpoints[bp].mask = mask;
	calc->z80.breakpoints[bp].testfunc = func;
	calc->z80.breakpoints[bp].testdata = data;
	calc->z80.breakpoints[bp].prev = 0;

	switch (type) {
	case TILEM_BREAK_MEM_READ:
		calc->z80.breakpoints[bp].next = calc->z80.breakpoint_mr;
		calc->z80.breakpoints[calc->z80.breakpoint_mr].prev = bp;
		calc->z80.breakpoint_mr = bp;
		break;

	case TILEM_BREAK_MEM_EXEC:
		calc->z80.breakpoints[bp].next = calc->z80.breakpoint_mx;
		calc->z80.breakpoints[calc->z80.breakpoint_mx].prev = bp;
		calc->z80.breakpoint_mx = bp;
		break;

	case TILEM_BREAK_MEM_WRITE:
		calc->z80.breakpoints[bp].next = calc->z80.breakpoint_mw;
		calc->z80.breakpoints[calc->z80.breakpoint_mw].prev = bp;
		calc->z80.breakpoint_mw = bp;
		break;

	case TILEM_BREAK_PORT_READ:
		calc->z80.breakpoints[bp].next = calc->z80.breakpoint_pr;
		calc->z80.breakpoints[calc->z80.breakpoint_pr].prev = bp;
		calc->z80.breakpoint_pr = bp;
		break;

	case TILEM_BREAK_PORT_WRITE:
		calc->z80.breakpoints[bp].next = calc->z80.breakpoint_pw;
		calc->z80.breakpoints[calc->z80.breakpoint_pw].prev = bp;
		calc->z80.breakpoint_pw = bp;
		break;

	case TILEM_BREAK_EXECUTE:
		calc->z80.breakpoints[bp].next = calc->z80.breakpoint_op;
		calc->z80.breakpoints[calc->z80.breakpoint_op].prev = bp;
		calc->z80.breakpoint_op = bp;
		break;

	default:
		tilem_internal(calc, "invalid bp type");
		bp_free(&calc->z80, bp);
		return 0;
	}

	return bp;
}

static int bptest_physical(TilemCalc* calc, dword addr, void* data)
{
	dword bpaddr = TILEM_PTR_TO_DWORD(data);
	dword maddr = (*calc->hw.mem_ltop)(calc, addr);

	return (!((bpaddr ^ maddr) & ~0x3fff));
}

int tilem_z80_add_breakpoint_physical(TilemCalc* calc, int type,
				      dword start, dword end)
{
	return tilem_z80_add_breakpoint(calc, type, start & 0x3fff,
					end & 0x3fff, 0x3fff,
					&bptest_physical,
					TILEM_DWORD_TO_PTR(start));
}

void tilem_z80_remove_breakpoint(TilemCalc* calc, int id)
{
	int prev, next;

	if (id < 1 || id > calc->z80.nbreakpoints
	    || !calc->z80.breakpoints[id].type) {
		tilem_internal(calc,
			       "attempt to remove invalid breakpoint %d", id);
		return;
	}

	prev = calc->z80.breakpoints[id].prev;
	next = calc->z80.breakpoints[id].next;
	calc->z80.breakpoints[prev].next = next;
	calc->z80.breakpoints[next].prev = prev;
	bp_free(&calc->z80, id);
}


static inline void check_timers(TilemCalc* calc)
{
	int tmr;
	dword t;
	TilemZ80TimerFunc callback;
	void* callbackdata;

	while (calc->z80.timer_cpu) {
		tmr = calc->z80.timer_cpu;
		t = calc->z80.clock - calc->z80.timers[tmr].count;
		if (t >= 10000)
			break;

		callback = calc->z80.timers[tmr].callback;
		callbackdata = calc->z80.timers[tmr].callbackdata;

		timer_unset(&calc->z80, tmr);
		timer_set(&calc->z80, tmr, calc->z80.timers[tmr].period,
			  calc->z80.timers[tmr].period, 0, t);

		(*callback)(calc, callbackdata);
	}

	while (calc->z80.timer_rt) {
		tmr = calc->z80.timer_rt;
		t = calc->z80.clock - calc->z80.timers[tmr].count;
		if (t >= 10000)
			break;

		callback = calc->z80.timers[tmr].callback;
		callbackdata = calc->z80.timers[tmr].callbackdata;

		timer_unset(&calc->z80, tmr);
		timer_set(&calc->z80, tmr, calc->z80.timers[tmr].period,
			  calc->z80.timers[tmr].period, 1, t);

		(*callback)(calc, callbackdata);
	}
}

static inline void check_breakpoints(TilemCalc* calc, int list, dword addr)
{
	dword masked;
	int bp;
	TilemZ80BreakpointFunc testfunc;
	void* testdata;

	for (bp = list; bp; bp = calc->z80.breakpoints[bp].next) {
		masked = addr & calc->z80.breakpoints[bp].mask;
		if (masked < calc->z80.breakpoints[bp].start
		    || masked > calc->z80.breakpoints[bp].end)
			continue;

		testfunc = calc->z80.breakpoints[bp].testfunc;
		testdata = calc->z80.breakpoints[bp].testdata;

		if (testfunc && !(*testfunc)(calc, addr, testdata))
			continue;

		calc->z80.stop_breakpoint = bp;
		tilem_z80_stop(calc, TILEM_STOP_BREAKPOINT);
	}
}

static inline byte z80_readb_m1(TilemCalc* calc, dword addr)
{
	byte b;
	addr &= 0xffff;
	b = (*calc->hw.z80_rdmem_m1)(calc, addr);
	check_breakpoints(calc, calc->z80.breakpoint_mx, addr);
	Rl++;
	return b;
}

static inline byte z80_readb(TilemCalc* calc, dword addr)
{
	byte b;
	addr &= 0xffff;
	b = (*calc->hw.z80_rdmem)(calc, addr);
	check_breakpoints(calc, calc->z80.breakpoint_mr, addr);
	return b;
}

static inline dword z80_readw(TilemCalc* calc, dword addr)
{
	dword v;
	addr &= 0xffff;
	v = (*calc->hw.z80_rdmem)(calc, addr);
	check_breakpoints(calc, calc->z80.breakpoint_mr, addr);
	addr = (addr + 1) & 0xffff;
	v |= (*calc->hw.z80_rdmem)(calc, addr) << 8;
	check_breakpoints(calc, calc->z80.breakpoint_mr, addr);
	return v;
}

static inline byte z80_input(TilemCalc* calc, dword addr)
{
	byte b;
	addr &= 0xffff;
	b = (*calc->hw.z80_in)(calc, addr);
	check_breakpoints(calc, calc->z80.breakpoint_pr, addr);
	return b;
}

static inline void z80_writeb(TilemCalc* calc, dword addr, byte value)
{
	addr &= 0xffff;
	(*calc->hw.z80_wrmem)(calc, addr, value);
	check_breakpoints(calc, calc->z80.breakpoint_mw, addr);
	calc->z80.lastwrite = calc->z80.clock;
}

static inline void z80_writew(TilemCalc* calc, dword addr, word value)
{
	addr &= 0xffff;
	(*calc->hw.z80_wrmem)(calc, addr, value);
	check_breakpoints(calc, calc->z80.breakpoint_mw, addr);
	addr = (addr + 1) & 0xffff;
	value >>= 8;
	(*calc->hw.z80_wrmem)(calc, addr, value);
	check_breakpoints(calc, calc->z80.breakpoint_mw, addr);
	calc->z80.lastwrite = calc->z80.clock;
}

static inline void z80_output(TilemCalc* calc, dword addr, byte value)
{
	addr &= 0xffff;
	(*calc->hw.z80_out)(calc, addr, value);
	check_breakpoints(calc, calc->z80.breakpoint_pw, addr);
}

#define readb_m1(aaa)     z80_readb_m1(calc, aaa)
#define readb(aaa)        z80_readb(calc, aaa)
#define readw(aaa)        z80_readw(calc, aaa)
#define input(aaa)        z80_input(calc, aaa)
#define writeb(aaa, vvv)  z80_writeb(calc, aaa, vvv)
#define writew(aaa, vvv)  z80_writew(calc, aaa, vvv)
#define output(aaa, vvv)  z80_output(calc, aaa, vvv)
#define delay(nnn)        calc->z80.clock += (nnn)

#include "z80cmds.h"

static dword z80_execute_opcode(TilemCalc* calc, byte op)
{
	byte tmp1;
	word tmp2;
	int offs;
#ifdef DISABLE_Z80_WZ_REGISTER
	TilemZ80Reg temp_wz, temp_wz2;
#endif

 opcode_main:
#include "z80main.h"
	return op;

 opcode_cb:
#include "z80cb.h"
	return op | 0xcb00;

 opcode_ed:
#include "z80ed.h"
	return op | 0xed00;

#define PREFIX_DD
 opcode_dd:
#include "z80ddfd.h"
	return op | 0xdd00;
 opcode_ddcb:
#include "z80cb.h"
	return op | 0xddcb0000;
#undef PREFIX_DD

#define PREFIX_FD
 opcode_fd:
#include "z80ddfd.h"
	return op | 0xfd00;
 opcode_fdcb:
#include "z80cb.h"
	return op | 0xfdcb0000;
#undef PREFIX_FD
}

static void z80_execute(TilemCalc* calc)
{
	TilemZ80* z80 = &calc->z80;
	byte busbyte;
	dword op;
	dword t1, t2;

	z80->stopping = 0;
	z80->stop_reason = 0;
	z80->stop_breakpoint = 0;

	if (!z80->timer_cpu && !z80->timer_rt) {
		tilem_internal(calc, "No timers set");
		return;
	}

	while (!z80->stopping) {
		op = (*calc->hw.z80_rdmem_m1)(calc, PC);
		PC++;
		Rl++;
		op = z80_execute_opcode(calc, op);
		check_breakpoints(calc, z80->breakpoint_op, op);
		check_timers(calc);

		if (z80->interrupts && IFF1 && op != 0xfb
		    && op != 0xddfb && op != 0xfdfb) {
			IFF1 = IFF2 = 0;
			Rl++;

			/* Depending on the calculator, this value
			   varies somewhat randomly from one interrupt
			   to the next (making IM 2 rather difficult
			   to use, and IM 0 essentially worthless.)
			   Most likely, there is nothing connected to
			   the data bus at interrupt time.  I seem to
			   remember somebody (sigma, perhaps?)
			   experimenting with this on the TI-83+ and
			   finding it usually 3F, 7F, BF, or FF.  Or
			   maybe I'm completely wrong.  In any case it
			   is unwise for programs to depend on this
			   value! */

			busbyte = rand() & 0xff;

			switch (IM) {
			case 0:
				delay(2);
				z80_execute_opcode(calc, busbyte);
				break;

			case 1:
				push(PC);
				PC = 0x0038;
				delay(13);
				break;

			case 2:
				/* FIXME: does accepting an IM 2
				   interrupt affect WZ?  It seems very
				   likely. */
				push(PC);
				PC = readw((IR & 0xff00) | busbyte);
				delay(19);
			}
			check_breakpoints(calc, z80->breakpoint_mx, PC);
		}
		else if (op != 0x76) {
			check_breakpoints(calc, z80->breakpoint_mx, PC);
		}
		else {
			PC--;
			if (z80->stopping)
				break;

			/* CPU halted: fast-forward to next timer event */
			if (z80->timer_cpu && z80->timer_rt) {
				t1 = (z80->timers[z80->timer_cpu].count
				      - z80->clock);
				t2 = (z80->timers[z80->timer_rt].count
				      - z80->clock);
				if (t1 > t2)
					t1 = t2;
			}
			else if (z80->timer_cpu) {
				t1 = (z80->timers[z80->timer_cpu].count
				      - z80->clock);
			}
			else if (z80->timer_rt) {
				t1 = (z80->timers[z80->timer_rt].count
				      - z80->clock);
			}
			else {
				tilem_internal(calc, "No timers set");
				return;
			}

			z80->clock += t1 & ~3;
			Rl += t1 / 4;
			check_timers(calc);
		}
	}
}

static void tmr_stop(TilemCalc* calc, void* data TILEM_ATTR_UNUSED)
{
	tilem_z80_stop(calc, TILEM_STOP_TIMEOUT);
}

dword tilem_z80_run(TilemCalc* calc, int clocks, int* remaining)
{
	int tmr = tilem_z80_add_timer(calc, clocks, 0, 0, &tmr_stop, 0);
	z80_execute(calc);
	if (remaining)
		*remaining = tilem_z80_get_timer_clocks(calc, tmr);
	tilem_z80_remove_timer(calc, tmr);
	return calc->z80.stop_reason;
}

dword tilem_z80_run_time(TilemCalc* calc, int microseconds, int* remaining)
{
	int tmr = tilem_z80_add_timer(calc, microseconds, 0, 1, &tmr_stop, 0);
	z80_execute(calc);
	if (remaining)
		*remaining = tilem_z80_get_timer_microseconds(calc, tmr);
	tilem_z80_remove_timer(calc, tmr);
	return calc->z80.stop_reason;
}
