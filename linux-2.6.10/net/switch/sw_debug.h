#ifndef _SW_DEBUG_H
#define _SW_DEBUG_H

#ifdef DEBUG
#define dbg(text,par...) printk(KERN_DEBUG text, ##par)
#else
#define dbg(par...)
#endif

#endif
