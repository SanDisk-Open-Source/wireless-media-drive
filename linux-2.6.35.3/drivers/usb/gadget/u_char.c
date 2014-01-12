/*                                                                          
 * u_char.c - USB character device glue                                     
 *                                                                          
 * Copyright (C) 2009-2010 Nokia Corporation                                
 * Author: Roger Quadros <roger.quadros@xxxxxxxxx>                          
 *                                                                          
 * This program is free software; you can redistribute it and/or modify     
 * it under the terms of the GNU General Public License as published by     
 * the Free Software Foundation; either version 2 of the License, or        
 * (at your option) any later version.                                      
 *                                                                          
 * This program is distributed in the hope that it will be useful,          
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            
 * GNU General Public License for more details.                             
 *                                                                          
 * You should have received a copy of the GNU General Public License        
 * along with this program; if not, write to the Free Software              
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA  
 */                                                                         
                                                                            
#include <linux/poll.h>                                                     
#include <linux/list.h>                                                     
#include <linux/vmalloc.h>                                                  
#include <linux/interrupt.h>                                                
#include <linux/kfifo.h>                                                    
#include "u_char.h"                                                         
                                                                            
/* Max simultaneous gchar devices. Increase if you need more */             
#define GC_MAX_DEVICES		4                                                  
                                                                            
/* Number of USB requests that can be queued at a time */                   
#define GC_QUEUE_SIZE		16                                                 
                                                                            
/* size in bytes of RX and TX FIFOs */                                      
#define GC_BUF_SIZE		65536                                                
                                                                            
/*----------------USB glue----------------------------------*/              
/*                                                                          
 * gc_alloc_req                                                             
 *                                                                          
 * Allocate a usb_request and its buffer.  Returns a pointer to the         
 * usb_request or NULL if there is an error.                                
 */                                                                         
struct usb_request *                                                        
gc_alloc_req(struct usb_ep *ep, unsigned len, gfp_t kmalloc_flags)          
{                                                                           
	struct usb_request *req;                                                   
                                                                            
	req = usb_ep_alloc_request(ep, kmalloc_flags);                             
                                                                            
	if (req != NULL) {                                                         
		req->length = len;                                                       
		req->buf = kmalloc(len, kmalloc_flags);                                  
		if (req->buf == NULL) {                                                  
			usb_ep_free_request(ep, req);                                          
			return NULL;                                                           
		}                                                                        
	}                                                                          
                                                                            
	return req;                                                                
}                                                                           
                                                                            
/*                                                                          
 * gc_free_req                                                              
 *                                                                          
 * Free a usb_request and its buffer.                                       
 */                                                                         
void gc_free_req(struct usb_ep *ep, struct usb_request *req)                
{                                                                           
	kfree(req->buf);                                                           
	usb_ep_free_request(ep, req);                                              
}                                                                           
                                                                            
static int gc_alloc_requests(struct usb_ep *ep, struct list_head *head,     
		void (*fn)(struct usb_ep *, struct usb_request *))                       
{                                                                           
	int                     i;                                                 
	struct usb_request      *req;                                              
                                                                            
	/* Pre-allocate up to GC_QUEUE_SIZE transfers, but if we can't             
	 * do quite that many this time, don't fail ... we just won't              
	 * be as speedy as we might otherwise be.                                  
	 */                                                                        
	for (i = 0; i < GC_QUEUE_SIZE; i++) {                                      
		req = gc_alloc_req(ep, ep->maxpacket, GFP_ATOMIC);                       
		if (!req)                                                                
			return list_empty(head) ? -ENOMEM : 0;                                 
		req->complete = fn;                                                      
		list_add_tail(&req->list, head);                                         
	}                                                                          
	return 0;                                                                  
}                                                                           
                                                                            
static void gc_free_requests(struct usb_ep *ep, struct list_head *head)     
{                                                                           
	struct usb_request      *req;                                              
                                                                            
	while (!list_empty(head)) {                                                
		req = list_entry(head->next, struct usb_request, list);                  
		list_del(&req->list);                                                    
		gc_free_req(ep, req);                                                    
	}                                                                          
}                                                                           
                                                                            
/*----------------------------------------------------------------*/        
                                                                            
struct gc_req {                                                             
	struct usb_request	*r;                                                    
	ssize_t			l;                                                             
	wait_queue_head_t	req_wait;                                                
};                                                                          
                                                                            
struct gc_dev {                                                             
	struct gchar		*gchar;                                                    
	struct device		*dev;		/* Driver model state */                           
	spinlock_t		lock;		/* serialize access */                               
	int			opened;		/* indicates if device open */                           
	wait_queue_head_t	close_wait;	/* wait for device close */                  
	int			index;		/* device index */                                       
                                                                            
	spinlock_t		rx_lock;	/* guard rx stuff */                               
	struct kfifo		rx_fifo;                                                   
	void			*rx_fifo_buf;                                                    
	struct list_head	rx_pool;                                                 
	struct tasklet_struct	rx_task;                                             
	wait_queue_head_t	rx_wait;	/* wait for data in RX buf */                  
	unsigned int		rx_queued;	/* no. of queued requests */                   
                                                                            
	spinlock_t		tx_lock;	/* guard tx stuff */                               
	struct kfifo		tx_fifo;                                                   
	void			*tx_fifo_buf;                                                    
	struct list_head	tx_pool;                                                 
	wait_queue_head_t	tx_wait;	/* wait for space in TX buf */                 
	unsigned int		tx_flush:1;	/* flush TX buf */                             
	wait_queue_head_t	tx_flush_wait;                                           
	int			tx_last_size;	/*last tx packet's size*/                            
	struct tasklet_struct	tx_task;                                             
};                                                                          
                                                                            
struct gc_data {                                                            
	struct gc_dev			*gcdevs;                                                 
	u8				nr_devs;                                                         
	struct class			*class;                                                  
	dev_t				dev;                                                           
	struct cdev			chdev;                                                     
	struct usb_gadget		*gadget;                                               
};                                                                          
                                                                            
static struct gc_data gcdata;                                               
                                                                            
static void gc_rx_complete(struct usb_ep *ep, struct usb_request *req);     
static void gc_tx_complete(struct usb_ep *ep, struct usb_request *req);     
static int gc_do_rx(struct gc_dev *gc);                                     
                                                                            
                                                                            
/*----------some more USB glue---------------------------*/                 
                                                                            
/* OUT complete, we have new data to read */                                
static void gc_rx_complete(struct usb_ep *ep, struct usb_request *req)      
{                                                                           
	struct gc_dev	*gc = ep->driver_data;                                       
	unsigned long	flags;                                                       
	int		i;                                                                   
                                                                            
	spin_lock_irqsave(&gc->rx_lock, flags);                                    
                                                                            
	/* put received data into RX ring buffer */                                
	/* we assume enough space is there in RX buffer for this request           
	 * the checking should be done in gc_do_rx() before this request           
	 * was queued */                                                           
	switch (req->status) {                                                     
	case 0:                                                                    
		/* normal completion */                                                  
		i = kfifo_in(&gc->rx_fifo, req->buf, req->actual);                       
		if (i != req->actual) {                                                  
			WARN(1, KERN_ERR "%s: PUT(%d) != actual(%d) data "                     
				"loss possible. rx_queued = %d\n", __func__, i,                      
						req->actual, gc->rx_queued);                                     
		}                                                                        
		gc->rx_queued--;                                                         
		dev_vdbg(gc->dev,                                                        
			"%s: rx len=%d, 0x%02x 0x%02x 0x%02x ...\n", __func__,                 
				req->actual, *((u8 *)req->buf),                                      
				*((u8 *)req->buf+1), *((u8 *)req->buf+2));                           
                                                                            
		/* wake up rx_wait */                                                    
		wake_up_interruptible(&gc->rx_wait);                                     
		break;                                                                   
	case -ESHUTDOWN:                                                           
		/* disconnect */                                                         
		dev_warn(gc->dev, "%s: %s shutdown\n", __func__, ep->name);              
		break;                                                                   
	default:                                                                   
		/* presumably a transient fault */                                       
		dev_warn(gc->dev, "%s: unexpected %s status %d\n",                       
				__func__, ep->name, req->status);                                    
		break;                                                                   
	}                                                                          
                                                                            
	/* recycle req back to rx pool */                                          
	list_add_tail(&req->list, &gc->rx_pool);                                   
	spin_unlock_irqrestore(&gc->rx_lock, flags);                               
	tasklet_schedule(&gc->rx_task);                                            
}                                                                           
                                                                            
static int gc_do_tx(struct gc_dev *gc);                                     
/* IN complete, i.e. USB write complete. we can free buffer */              
static void gc_tx_complete(struct usb_ep *ep, struct usb_request *req)      
{                                                                           
	struct gc_dev	*gc = ep->driver_data;                                       
	unsigned long	flags;                                                       
                                                                            
	spin_lock_irqsave(&gc->tx_lock, flags);                                    
	/* recycle back to pool */                                                 
	list_add_tail(&req->list, &gc->tx_pool);                                   
	spin_unlock_irqrestore(&gc->tx_lock, flags);                               
                                                                            
	switch (req->status) {                                                     
	case 0:                                                                    
		/* normal completion, queue next request */                              
		tasklet_schedule(&gc->tx_task);                                          
		break;                                                                   
	case -ESHUTDOWN:                                                           
		/* disconnect */                                                         
		dev_warn(gc->dev, "%s: %s shutdown\n", __func__, ep->name);              
		break;                                                                   
	default:                                                                   
		/* presumably a transient fault */                                       
		dev_warn(gc->dev, "%s: unexpected %s status %d\n",                       
				__func__, ep->name, req->status);                                    
		break;                                                                   
	}                                                                          
}                                                                           
                                                                            
                                                                            
/* Read the TX buffer and send to USB */                                    
/* gc->tx_lock must be held */                                              
static int gc_do_tx(struct gc_dev *gc)                                      
{                                                                           
	struct list_head	*pool	= &gc->tx_pool;                                    
	struct usb_ep		*in	= gc->gchar->ep_in;                                    
	int			status;                                                            
                                                                            
	if (!in)                                                                   
		return -ENODEV;                                                          
                                                                            
	while (!list_empty(pool)) {                                                
		struct	usb_request	*req;                                                
		int	len;                                                                 
                                                                            
		req = list_entry(pool->next, struct usb_request, list);                  
                                                                            
		len = kfifo_len(&gc->tx_fifo);                                           
		if (!len && !gc->tx_flush)                                               
			/* TX buf empty */                                                     
			break;                                                                 
                                                                            
		list_del(&req->list);                                                    
                                                                            
		req->zero = 0;                                                           
		if (len > in->maxpacket) {                                               
			len = in->maxpacket;                                                   
			gc->tx_last_size = 0;	/* not the last packet */                        
		} else {                                                                 
			/* this is last packet in TX buf. send ZLP/SLP                         
			 * if user has requested so                                            
			 */                                                                    
			req->zero = gc->tx_flush;                                              
			gc->tx_last_size = len;                                                
		}                                                                        
                                                                            
		len = kfifo_out(&gc->tx_fifo, req->buf, len);                            
		req->length = len;                                                       
                                                                            
		if (req->zero) {                                                         
			gc->tx_flush = 0;                                                      
			wake_up_interruptible(&gc->tx_flush_wait);                             
		}                                                                        
                                                                            
		dev_vdbg(gc->dev,                                                        
			"%s: tx len=%d, 0x%02x 0x%02x 0x%02x ...\n", __func__,                 
				len, *((u8 *)req->buf),                                              
				*((u8 *)req->buf+1), *((u8 *)req->buf+2));                           
		/* Drop lock while we call out of driver; completions                    
		 * could be issued while we do so.  Disconnection may                    
		 * happen too; maybe immediately before we queue this!                   
		 *                                                                       
		 * NOTE that we may keep sending data for a while after                  
		 * the file is closed.                                                   
		 */                                                                      
		spin_unlock(&gc->tx_lock);                                               
		status = usb_ep_queue(in, req, GFP_ATOMIC);                              
		spin_lock(&gc->tx_lock);                                                 
                                                                            
		if (status) {                                                            
			dev_err(gc->dev, "%s: %s %s err %d\n",                                 
					__func__, "queue", in->name, status);                              
			list_add(&req->list, pool);                                            
			break;                                                                 
		}                                                                        
                                                                            
		/* abort immediately after disconnect */                                 
		if (!gc->gchar) {                                                        
			dev_dbg(gc->dev,                                                       
				"%s: disconnected so aborting\n", __func__);                         
			break;                                                                 
		}                                                                        
                                                                            
		/* wake up tx_wait */                                                    
		wake_up_interruptible(&gc->tx_wait);                                     
	}                                                                          
	return 0;                                                                  
}                                                                           
                                                                            
static void gc_tx_task(unsigned long _gc)                                   
{                                                                           
	struct gc_dev	*gc = (void *)_gc;                                           
                                                                            
	spin_lock_irq(&gc->tx_lock);                                               
	if (gc->gchar && gc->gchar->ep_in)                                         
		gc_do_tx(gc);                                                            
	spin_unlock_irq(&gc->tx_lock);                                             
}                                                                           
                                                                            
/* Tasklet:  Queue USB read requests whenever RX buffer available           
 *	Must be called with gc->rx_lock held                                     
 */                                                                         
static int gc_do_rx(struct gc_dev *gc)                                      
{                                                                           
	/* Queue the request only if required space is there in RX buffer */       
	struct list_head	*pool	= &gc->rx_pool;                                    
	struct usb_ep		*out	= gc->gchar->ep_out;                                 
	int			started = 0;                                                       
                                                                            
	if (!out)                                                                  
		return -EINVAL;                                                          
                                                                            
	while (!list_empty(pool)) {                                                
		struct usb_request      *req;                                            
		int                     status;                                          
                                                                            
		req = list_entry(pool->next, struct usb_request, list);                  
		list_del(&req->list);                                                    
		req->length = out->maxpacket;                                            
                                                                            
		/* check if space is available in RX buf for this request */             
		if (kfifo_avail(&gc->rx_fifo) <                                          
				(gc->rx_queued + 2)*req->length) {                                   
			/* insufficient space, recycle req */                                  
			list_add(&req->list, pool);                                            
			break;                                                                 
		}                                                                        
		gc->rx_queued++;                                                         
                                                                            
		/* drop lock while we call out; the controller driver                    
		 * may need to call us back (e.g. for disconnect)                        
		 */                                                                      
		spin_unlock(&gc->rx_lock);                                               
		status = usb_ep_queue(out, req, GFP_ATOMIC);                             
		spin_lock(&gc->rx_lock);                                                 
                                                                            
		if (status) {                                                            
			dev_warn(gc->dev, "%s: %s %s err %d\n",                                
					__func__, "queue", out->name, status);                             
			list_add(&req->list, pool);                                            
			break;                                                                 
		}                                                                        
                                                                            
		started++;                                                               
                                                                            
		/* abort immediately after disconnect */                                 
		if (!gc->gchar) {                                                        
			dev_dbg(gc->dev, "%s: disconnected so aborting\n",                     
					__func__);                                                         
			break;                                                                 
		}                                                                        
	}                                                                          
	return started;                                                            
}                                                                           
                                                                            
                                                                            
static void gc_rx_task(unsigned long _gc)                                   
{                                                                           
	struct gc_dev	*gc = (void *)_gc;                                           
                                                                            
	spin_lock_irq(&gc->rx_lock);                                               
	if (gc->gchar && gc->gchar->ep_out)                                        
		gc_do_rx(gc);                                                            
	spin_unlock_irq(&gc->rx_lock);                                             
}                                                                           
                                                                            
/*----------FILE Operations-------------------------------*/                
                                                                            
static int gc_open(struct inode *inode, struct file *filp)                  
{                                                                           
	unsigned	minor = iminor(inode);                                           
	struct gc_dev	*gc;                                                         
	int		index;                                                               
                                                                            
	index = minor - MINOR(gcdata.dev);                                         
	if (index >= gcdata.nr_devs)                                               
		return -ENODEV;                                                          
                                                                            
	if (!gcdata.gcdevs)                                                        
		return -ENODEV;                                                          
                                                                            
	if (!gcdata.gcdevs[index].gchar)                                           
		return -ENODEV;                                                          
                                                                            
	filp->private_data = &gcdata.gcdevs[index];                                
	gc = filp->private_data;                                                   
                                                                            
	/* prevent multiple opens for now */                                       
	if (gc->opened)                                                            
		return -EBUSY;                                                           
	spin_lock_irq(&gc->lock);                                                  
	if (gc->opened) {                                                          
		spin_unlock_irq(&gc->lock);                                              
		return -EBUSY;                                                           
	}                                                                          
	gc->opened = 1;                                                            
	spin_unlock_irq(&gc->lock);                                                
	gc->index = index;                                                         
                                                                            
	if (!gc->tx_fifo_buf && gc->gchar->ep_in) {                                
		gc->tx_fifo_buf = vmalloc(GC_BUF_SIZE);                                  
		if (gc->tx_fifo_buf == NULL)                                             
			return -ENOMEM;                                                        
		kfifo_init(&gcdata.gcdevs[minor].tx_fifo,                                
				gc->tx_fifo_buf, GC_BUF_SIZE);                                       
	}                                                                          
                                                                            
	if (!gc->rx_fifo_buf && gc->gchar->ep_out) {                               
		gc->rx_fifo_buf = vmalloc(GC_BUF_SIZE);                                  
		if (gc->rx_fifo_buf == NULL) {                                           
			vfree(gc->tx_fifo_buf);                                                
			return -ENOMEM;                                                        
		}                                                                        
		kfifo_init(&gcdata.gcdevs[minor].rx_fifo,                                
				gc->rx_fifo_buf, GC_BUF_SIZE);                                       
	}                                                                          
                                                                            
	if (gc->gchar && gc->gchar->open)                                          
		gc->gchar->open(gc->gchar);                                              
                                                                            
	/* if connected, start receiving */                                        
	if (gc->gchar)                                                             
		tasklet_schedule(&gc->rx_task);                                          
                                                                            
	dev_dbg(gc->dev, "%s: gc%d opened\n", __func__, gc->index);                
	return 0;                                                                  
}                                                                           
                                                                            
static int gc_release(struct inode *inode, struct file *filp)               
{                                                                           
	struct gc_dev			*gc = filp->private_data;                                
                                                                            
	filp->private_data = NULL;                                                 
                                                                            
	dev_dbg(gc->dev, "%s: releasing gc%d\n", __func__, gc->index);             
                                                                            
	if (!gc->opened)                                                           
		goto gc_release_exit;                                                    
                                                                            
	vfree(gc->tx_fifo_buf);                                                    
	gc->tx_fifo_buf = NULL;                                                    
	vfree(gc->rx_fifo_buf);                                                    
	gc->rx_fifo_buf = NULL;                                                    
                                                                            
	if (gc->gchar && gc->gchar->close)                                         
		gc->gchar->close(gc->gchar);                                             
                                                                            
	spin_lock_irq(&gc->lock);                                                  
	gc->opened = 0;                                                            
	spin_unlock_irq(&gc->lock);                                                
                                                                            
	wake_up_interruptible(&gc->close_wait);                                    
                                                                            
gc_release_exit:                                                            
	dev_dbg(gc->dev, "%s: gc%d released!!\n", __func__, gc->index);            
	return 0;                                                                  
}                                                                           
                                                                            
static int gc_can_read(struct gc_dev *gc)                                   
{                                                                           
	int ret;                                                                   
                                                                            
	spin_lock_irq(&gc->rx_lock);                                               
	ret = kfifo_len(&gc->rx_fifo) ? 1 : 0;                                     
	spin_unlock_irq(&gc->rx_lock);                                             
                                                                            
	return ret;                                                                
}                                                                           
                                                                            
static ssize_t gc_read(struct file *filp, char __user *buff,                
				size_t len, loff_t *o)                                               
{                                                                           
	struct	gc_dev	*gc = filp->private_data;                                  
	int	read = 0;
    int readout;                                                              
                                                                            
	if (!gc->gchar || !gc->gchar->ep_out) {                                    
		/* not yet connected or reading not possible*/                           
		return -EINVAL;                                                          
	}                                                                          
                                                                            
	if (len) {                                                                 
		read = kfifo_len(&gc->rx_fifo);                                          
		if (!read) {                                                             
			/* if NONBLOCK then return immediately */                              
			if (filp->f_flags & O_NONBLOCK)                                        
				return -EAGAIN;                                                      
                                                                            
			/* sleep till we have some data */                                     
			if (wait_event_interruptible(gc->rx_wait,                              
							gc_can_read(gc)))                                              
				return -ERESTARTSYS;                                                 
                                                                            
		}                   
        // johnson                                                     
        //read = kfifo_to_user(&gc->rx_fifo, buff, len);                           
		read = kfifo_to_user(&gc->rx_fifo, buff, len, &readout);                           
	}                                                                          
                                                                            
	if (read > 0) {                                                            
		spin_lock_irq(&gc->rx_lock);                                             
		gc_do_rx(gc);                                                            
		spin_unlock_irq(&gc->rx_lock);                                           
	}                                                                          
                                                                            
	dev_vdbg(gc->dev, "%s done %d/%d\n", __func__, read, len);                 
	return read;                                                               
}                                                                           
                                                                            
static int gc_can_write(struct gc_dev *gc)                                  
{                                                                           
	int ret;                                                                   
                                                                            
	spin_lock_irq(&gc->tx_lock);                                               
	ret = !kfifo_is_full(&gc->tx_fifo);                                        
	spin_unlock_irq(&gc->tx_lock);                                             
                                                                            
	return ret;                                                                
}                                                                           
                                                                            
static ssize_t gc_write(struct file *filp, const char __user *buff,         
						size_t len, loff_t *o)                                           
{                                                                           
	struct	gc_dev	*gc = filp->private_data;                                  
	int	wrote = 0;     
    int readout;                                                        
                                                                            
	if (!gc->gchar || !gc->gchar->ep_in) {                                     
		/* not yet connected or writing not possible */                          
		return -EINVAL;                                                          
	}                                                                          
                                                                            
	if (len) {                                                                 
		if (kfifo_is_full(&gc->tx_fifo)) {                                       
			if (filp->f_flags & O_NONBLOCK)                                        
				return -EAGAIN;                                                      
                                                                            
			/* sleep till we have some space to write into */                      
			if (wait_event_interruptible(gc->tx_wait,                              
						gc_can_write(gc)))                                               
				return -ERESTARTSYS;                                                 
                                                                            
		}                                                                        
        // johnson
		//wrote = kfifo_from_user(&gc->tx_fifo, buff, len);
        wrote = kfifo_from_user(&gc->tx_fifo, buff, len, &readout);                                                
		if (wrote < 0)                                                           
			dev_warn(gc->dev, "%s fault %d\n", __func__, wrote);                   
	}                                                                          
                                                                            
	if (wrote > 0) {                                                           
		spin_lock_irq(&gc->tx_lock);                                             
		gc_do_tx(gc);                                                            
		spin_unlock_irq(&gc->tx_lock);                                           
	}                                                                          
                                                                            
	dev_vdbg(gc->dev, "%s done %d/%d\n", __func__, wrote, len);                
	return wrote;                                                              
}                                                                           
                                                                            
static long gc_ioctl(struct file *filp, unsigned code, unsigned long value) 
{                                                                           
	struct gc_dev			*gc = filp->private_data;                                
	int status = -EINVAL;                                                      
                                                                            
	if (gc->gchar && gc->gchar->ioctl)                                         
		status = gc->gchar->ioctl(gc->gchar, code, value);                       
                                                                            
	dev_dbg(gc->dev, "%s done\n", __func__);                                   
	return status;                                                             
}                                                                           
                                                                            
static unsigned int gc_poll(struct file *filp, struct poll_table_struct *pt)
{                                                                           
	struct gc_dev			*gc = filp->private_data;                                
	int				ret = 0;                                                         
	int				rx = 0, tx = 0;                                                  
                                                                            
	/* generic poll implementation */                                          
	poll_wait(filp, &gc->rx_wait, pt);                                         
	poll_wait(filp, &gc->tx_wait, pt);                                         
                                                                            
	if (!gc->gchar) {                                                          
		/* not yet connected */                                                  
		goto poll_exit;                                                          
	}                                                                          
                                                                            
	/* check if data is available to read */                                   
	spin_lock_irq(&gc->rx_lock);                                               
	if (gc->gchar->ep_out) {                                                   
		rx = kfifo_len(&gc->rx_fifo);                                            
		if (rx)                                                                  
			ret |= POLLIN | POLLRDNORM;                                            
	}                                                                          
	spin_unlock_irq(&gc->rx_lock);                                             
                                                                            
	/* check if space is available to write */                                 
	spin_lock_irq(&gc->tx_lock);                                               
	if (gc->gchar->ep_in) {                                                    
		tx = kfifo_avail(&gc->tx_fifo);                                          
		if (tx)                                                                  
			ret |= POLLOUT | POLLWRNORM;                                           
	}                                                                          
	spin_unlock_irq(&gc->tx_lock);                                             
                                                                            
	dev_dbg(gc->dev, "%s: rx avl %d, tx space %d\n", __func__, rx, tx);        
poll_exit:                                                                  
                                                                            
	return ret;                                                                
}                                                                           
                                                                            
int gc_fsync(struct file *filp, struct dentry *dentry, int datasync)        
{                                                                           
	struct gc_dev	*gc = filp->private_data;                                    
                                                                            
	if (!gc->gchar || !gc->gchar->ep_in) {                                     
		/* not yet connected or writing not possible */                          
		return -EINVAL;                                                          
	}                                                                          
                                                                            
	/* flush the TX buffer and send ZLP/SLP                                    
	 * we will wait till TX buffer is empty                                    
	 */                                                                        
	spin_lock_irq(&gc->tx_lock);                                               
                                                                            
	if (gc->tx_flush) {                                                        
		dev_err(gc->dev, "%s tx_flush already requested\n", __func__);           
		spin_unlock_irq(&gc->tx_lock);                                           
		return -EINVAL;                                                          
	}                                                                          
                                                                            
	if (!kfifo_len(&gc->tx_fifo)) {                                            
		if (gc->tx_last_size == gc->gchar->ep_in->maxpacket)                     
			gc->tx_flush = 1;                                                      
	} else                                                                     
		gc->tx_flush = 1;                                                        
                                                                            
	if (gc->tx_flush) {                                                        
		gc_do_tx(gc);                                                            
                                                                            
		spin_unlock_irq(&gc->tx_lock);                                           
                                                                            
		if (wait_event_interruptible(gc->tx_flush_wait,                          
					!gc->tx_flush))                                                    
			return -ERESTARTSYS;                                                   
	} else                                                                     
		spin_unlock_irq(&gc->tx_lock);                                           
                                                                            
	dev_dbg(gc->dev, "%s complete\n", __func__);                               
	return 0;                                                                  
}                                                                           
                                                                            
static const struct file_operations gc_fops = {                             
	.owner		= THIS_MODULE,                                                   
	.open		= gc_open,                                                         
	.poll		= gc_poll,                                                         
	.unlocked_ioctl	= gc_ioctl,                                                
	.release	= gc_release,                                                    
	.read		= gc_read,                                                         
	.write		= gc_write,                                                      
	.fsync		= gc_fsync,                                                      
};                                                                          
                                                                            
/*------------USB Gadget Driver Interface----------------------------*/     
                                                                            
/**                                                                         
 * gchar_setup - initialize the character driver for one or more devices    
 * @g: gadget to associate with these devices                               
 * @devs_num: number of character devices to support                        
 * Context: may sleep                                                       
 *                                                                          
 * This driver needs to know how many char. devices it should manage.       
 * Use this call to set up the devices that will be exported through USB.   
 * Later, connect them to functions based on what configuration is activated
 * by the USB host, and disconnect them as appropriate.                     
 *                                                                          
 * Returns negative errno or zero.                                          
 */                                                                         
int __init gchar_setup(struct usb_gadget *g, u8 devs_num)                   
{                                                                           
	int		status;                                                              
	int		i = 0;                                                               
                                                                            
	if (gcdata.nr_devs)                                                        
		return -EBUSY;                                                           
                                                                            
	if (devs_num == 0 || devs_num > GC_MAX_DEVICES)                            
		return -EINVAL;                                                          
                                                                            
	gcdata.gcdevs = kzalloc(sizeof(struct gc_dev) * devs_num, GFP_KERNEL);     
	if (!gcdata.gcdevs)                                                        
		return -ENOMEM;                                                          
                                                                            
	/* created char dev */                                                     
	status = alloc_chrdev_region(&gcdata.dev, 0, devs_num, "gchar");           
	if (status)                                                                
		goto fail1;                                                              
                                                                            
	cdev_init(&gcdata.chdev, &gc_fops);                                        
                                                                            
	gcdata.chdev.owner = THIS_MODULE;                                          
	gcdata.nr_devs	= devs_num;                                                
                                                                            
	status = cdev_add(&gcdata.chdev, gcdata.dev, devs_num);                    
	if (status)                                                                
		goto fail2;                                                              
                                                                            
	/* register with sysfs */                                                  
	gcdata.class = class_create(THIS_MODULE, "gchar");                         
	if (IS_ERR(gcdata.class)) {                                                
		pr_err("%s: could not create class gchar\n", __func__);                  
		status = PTR_ERR(gcdata.class);                                          
		goto fail3;                                                              
	}                                                                          
                                                                            
	for (i = 0; i < devs_num; i++) {                                           
		struct gc_dev	*gc;                                                       
                                                                            
		gc = &gcdata.gcdevs[i];                                                  
		spin_lock_init(&gc->lock);                                               
		spin_lock_init(&gc->rx_lock);                                            
		spin_lock_init(&gc->tx_lock);                                            
		INIT_LIST_HEAD(&gc->rx_pool);                                            
		INIT_LIST_HEAD(&gc->tx_pool);                                            
		init_waitqueue_head(&gc->rx_wait);                                       
		init_waitqueue_head(&gc->tx_wait);                                       
		init_waitqueue_head(&gc->tx_flush_wait);                                 
		init_waitqueue_head(&gc->close_wait);                                    
                                                                            
		tasklet_init(&gc->rx_task, gc_rx_task, (unsigned long) gc);              
		tasklet_init(&gc->tx_task, gc_tx_task, (unsigned long) gc);              
		gc->dev = device_create(gcdata.class, NULL,                              
			MKDEV(MAJOR(gcdata.dev), MINOR(gcdata.dev) + i),                       
			NULL, "gc%d", i);                                                      
		if (IS_ERR(gc->dev)) {                                                   
			pr_err("%s: device_create() failed for device %d\n",                   
					__func__, i);                                                      
			for ( ; i > 0; i--) {                                                  
				device_destroy(gcdata.class,                                         
						MKDEV(MAJOR(gcdata.dev),                                         
						MINOR(gcdata.dev) + i));                                         
			}                                                                      
			goto fail4;                                                            
		}                                                                        
	}                                                                          
                                                                            
	gcdata.gadget = g;                                                         
                                                                            
	return 0;                                                                  
                                                                            
fail4:                                                                      
	class_destroy(gcdata.class);                                               
fail3:                                                                      
	cdev_del(&gcdata.chdev);                                                   
fail2:                                                                      
	unregister_chrdev_region(gcdata.dev, gcdata.nr_devs);                      
fail1:                                                                      
	kfree(gcdata.gcdevs);                                                      
	gcdata.gcdevs = NULL;                                                      
	gcdata.nr_devs = 0;                                                        
                                                                            
	return status;                                                             
}                                                                           
                                                                            
static int gc_closed(struct gc_dev *gc)                                     
{                                                                           
	int ret;                                                                   
                                                                            
	spin_lock_irq(&gc->lock);                                                  
	ret = !gc->opened;                                                         
	spin_unlock_irq(&gc->lock);                                                
	return ret;                                                                
}                                                                           
                                                                            
/**                                                                         
 * gchar_cleanup - remove the USB to character devicer and devices          
 * Context: may sleep                                                       
 *                                                                          
 * This is called to free all resources allocated by @gchar_setup().        
 * It may need to wait until some open /dev/ files have been closed.        
 */                                                                         
void gchar_cleanup(void)                                                    
{                                                                           
	int i;                                                                     
                                                                            
	if (!gcdata.gcdevs)                                                        
		return;                                                                  
                                                                            
	for (i = 0; i < gcdata.nr_devs; i++) {                                     
		struct gc_dev *gc = &gcdata.gcdevs[i];                                   
                                                                            
		tasklet_kill(&gc->rx_task);                                              
		tasklet_kill(&gc->tx_task);                                              
		device_destroy(gcdata.class, MKDEV(MAJOR(gcdata.dev),                    
				MINOR(gcdata.dev) + i));                                             
		/* wait till open files are closed */                                    
		wait_event(gc->close_wait, gc_closed(gc));                               
	}                                                                          
                                                                            
	cdev_del(&gcdata.chdev);                                                   
	class_destroy(gcdata.class);                                               
                                                                            
	/* cdev_put(&gchar>chdev); */                                              
	unregister_chrdev_region(gcdata.dev, gcdata.nr_devs);                      
                                                                            
	kfree(gcdata.gcdevs);                                                      
	gcdata.gcdevs = NULL;                                                      
	gcdata.nr_devs = 0;                                                        
}                                                                           
                                                                            
/**                                                                         
 * gchar_connect - notify the driver that USB link is active                
 * @gchar: the function, setup with endpoints and descriptors               
 * @num: the device number that is active                                   
 * @name: name of the function                                              
 * Context: any (usually from irq)                                          
 *                                                                          
 * This is called to activate the endpoints and let the driver know         
 * that USB link is active.                                                 
 *                                                                          
 * Caller needs to have set up the endpoints and USB function in @gchar     
 * before calling this, as well as the appropriate (speed-specific)         
 * endpoint descriptors, and also have set up the char driver by calling    
 * @gchar_setup().                                                          
 *                                                                          
 * Returns negative error or zeroa                                          
 * On success, ep->driver_data will be overwritten                          
 */                                                                         
int gchar_connect(struct gchar *gchar, u8 num, const char *name)            
{                                                                           
	int		status = 0;                                                          
	struct gc_dev	*gc;                                                         
                                                                            
	if (num >= gcdata.nr_devs) {                                               
		pr_err("%s: invalid device number\n", __func__);                         
		return -EINVAL;                                                          
	}                                                                          
                                                                            
	gc = &gcdata.gcdevs[num];                                                  
                                                                            
	dev_dbg(gc->dev, "%s %s %d\n", __func__, name, num);                       
                                                                            
	if (!gchar->ep_out && !gchar->ep_in) {                                     
		dev_err(gc->dev, "%s: Neither IN nor OUT endpoint available\n",          
								__func__);                                                   
		return -EINVAL;                                                          
	}                                                                          
                                                                            
	if (gchar->ep_out) {                                                       
		status = usb_ep_enable(gchar->ep_out, gchar->ep_out_desc);               
		if (status < 0)                                                          
			return status;                                                         
                                                                            
		gchar->ep_out->driver_data = gc;                                         
	}                                                                          
                                                                            
	if (gchar->ep_in) {                                                        
		status = usb_ep_enable(gchar->ep_in, gchar->ep_in_desc);                 
		if (status < 0)                                                          
			goto fail1;                                                            
                                                                            
		gchar->ep_in->driver_data = gc;                                          
	}                                                                          
                                                                            
	kfifo_reset(&gc->tx_fifo);                                                 
	kfifo_reset(&gc->rx_fifo);                                                 
	gc->rx_queued = 0;                                                         
	gc->tx_flush = 0;                                                          
	gc->tx_last_size = 0;                                                      
                                                                            
	if (gchar->ep_out) {                                                       
		status = gc_alloc_requests(gchar->ep_out, &gc->rx_pool,                  
						&gc_rx_complete);                                                
		if (status)                                                              
			goto fail2;                                                            
	}                                                                          
                                                                            
	if (gchar->ep_in) {                                                        
		status = gc_alloc_requests(gchar->ep_in, &gc->tx_pool,                   
						&gc_tx_complete);                                                
		if (status)                                                              
			goto fail3;                                                            
	}                                                                          
                                                                            
	/* connect gchar */                                                        
	gc->gchar = gchar;                                                         
                                                                            
	/* if userspace has opened the device, enable function */                  
	if (gc->opened)                                                            
		gc->gchar->open(gc->gchar);                                              
                                                                            
	/* if device is opened by user space then start RX */                      
	if (gc->opened)                                                            
		tasklet_schedule(&gc->rx_task);                                          
                                                                            
	dev_dbg(gc->dev, "%s complete\n", __func__);                               
	return 0;                                                                  
                                                                            
fail3:                                                                      
	if (gchar->ep_out)                                                         
		gc_free_requests(gchar->ep_out, &gc->rx_pool);                           
                                                                            
fail2:                                                                      
	if (gchar->ep_in) {                                                        
		gchar->ep_in->driver_data = NULL;                                        
		usb_ep_disable(gchar->ep_in);                                            
	}                                                                          
fail1:                                                                      
	if (gchar->ep_out) {                                                       
		gchar->ep_out->driver_data = NULL;                                       
		usb_ep_disable(gchar->ep_out);                                           
	}                                                                          
                                                                            
	return status;                                                             
}                                                                           
                                                                            
/**                                                                         
 * gchar_disconnect - notify the driver that USB link is inactive           
 * @gchar: the function, on which, gchar_connect() was called               
 * Context: any (usually from irq)                                          
 *                                                                          
 * this is called to deactivate the endpoints (related to @gchar)           
 * and let the driver know that the USB link is inactive                    
 */                                                                         
void gchar_disconnect(struct gchar *gchar)                                  
{                                                                           
	struct gc_dev *gc;                                                         
                                                                            
	if (!gchar->ep_out && !gchar->ep_in)                                       
		return;                                                                  
                                                                            
	if (gchar->ep_out)                                                         
		gc = gchar->ep_out->driver_data;                                         
	else                                                                       
		gc = gchar->ep_in->driver_data;                                          
                                                                            
	if (!gc) {                                                                 
		pr_err("%s Invalid gc_dev\n", __func__);                                 
		return;                                                                  
	}                                                                          
                                                                            
	spin_lock(&gc->lock);                                                      
                                                                            
	if (gchar->ep_out) {                                                       
		usb_ep_disable(gchar->ep_out);                                           
		gc_free_requests(gc->gchar->ep_out, &gc->rx_pool);                       
	}                                                                          
                                                                            
	if (gchar->ep_in) {                                                        
		usb_ep_disable(gchar->ep_in);                                            
		gc_free_requests(gc->gchar->ep_in, &gc->tx_pool);                        
	}                                                                          
                                                                            
	gc->gchar = NULL;                                                          
	gchar->ep_out->driver_data = NULL;                                         
	gchar->ep_in->driver_data = NULL;                                          
                                                                            
	spin_unlock(&gc->lock);                                                    
}                                                                           
