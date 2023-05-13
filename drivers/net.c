#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
EthernetFrame temp_buffers1[RXBD_CNT];
EthernetFrame temp_buffers2[RXBD_CNT];

uint64_t rx_len[RXBD_CNT];
uint64_t rx_len1[RXBD_CNT];
uint64_t rx_len2[RXBD_CNT];

int net_poll_mode;
int now_packet = 0;
int now_packet1 = 0;
int now_packet2 = 0;

extern LONG Rx[RXBD_CNT+1];
extern LONG Tx[TXBD_CNT+1];

int rx_curr = 0, rx_tail = 0;
int rx1_curr = 0, rx1_tail = 0;
int rx2_curr = 0, rx2_tail = 0;

list_node_t send_queue;
list_node_t recv_queue;

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    unsigned char* dest = (unsigned char*)addr;
    unsigned char* src;

    if(net_poll_mode==4)
    {
        if(now_packet1 + now_packet2 == 0)
            EmacPsRecv(&EmacPsInstance, rx_buffers, 32);
        //EmacPsRecv(&EmacPsInstance, rx_buffers, 1);
        current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
        if(current_running->port==1)
        {
            while(now_packet1==0)
            {
                current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
                do_block(&(current_running->list),&recv_queue);
                do_scheduler();
            }
            src = (unsigned char*)temp_buffers1[rx1_curr];
            for(int j=0;j<rx_len1[rx1_curr];j++)
            {
                *dest = *src;
                dest++;
                src++;
            }
            *frLength = rx_len1[rx1_curr];
            rx_len1[rx1_curr] = 0;
            rx1_curr = (rx1_curr+1)%RXBD_CNT;
            now_packet1--;

            screen_move_cursor(1, 18);
            prints("port 1 : %d (%d,%d)", now_packet1, rx1_curr, rx1_tail);

        }
        else if(current_running->port==2)
        {
            while(now_packet2==0)
            {
                current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
                do_block(&(current_running->list),&recv_queue);
                do_scheduler();
            }
            src = (unsigned char*)temp_buffers2[rx2_curr];
            for(int j=0;j<rx_len2[rx2_curr];j++)
            {
                *dest = *src;
                dest++;
                src++;
            }
            *frLength = rx_len2[rx2_curr];
            rx_len2[rx2_curr] = 0;
            rx2_curr = (rx2_curr+1)%RXBD_CNT;
            now_packet2--;

            screen_move_cursor(1, 19);
            prints("port 2 : %d (%d,%d)", now_packet2, rx2_curr, rx2_tail);
        }
        u32 Reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
        XEMACPS_RXSR_OFFSET);
        u32 Reg2 = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
        XEMACPS_ISR_OFFSET);
        screen_move_cursor(1, 17);
        prints("%x : %x", Reg, Reg2);
        return 1;
    }
    while(num_packet)
    {
        if(num_packet>=32)
        {
            for(int i=0;i<32;i++)
                Rx[i] = 0;
            EmacPsRecv(&EmacPsInstance, rx_buffers, 32);
            now_packet=0;
            if(net_poll_mode!=0)
            {
                while(now_packet<32)
                {
                    now_packet=0;
                    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
                    do_block(&(current_running->list),&recv_queue);
                    do_scheduler();
                    
                    EmacPsWaitRecv(&EmacPsInstance, 32, rx_len);
                }
                //screen_move_cursor(30, 1);
                //prints("now:%d\n", now_packet);
            }
            else
                EmacPsWaitRecv(&EmacPsInstance, 32, rx_len);
            for(int i=0; i<32; i++)
            {
                src=(unsigned char*)rx_buffers[i];
                for(int j=0;j<rx_len[i];j++)
                {
                    *dest = *src;
                    dest++;
                    src++;
                }
                *frLength = rx_len[i];
                frLength++;
                rx_len[i] = 0;
                //XEmacPs_BdClearRxNew(Rx+i);
            }
            num_packet-=32;
            u32 Reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                XEMACPS_RXSR_OFFSET);
            XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, 
                XEMACPS_RXSR_OFFSET, 
                Reg | XEMACPS_RXSR_FRAMERX_MASK);
        }
        else
        {
            for(int i=0;i<32;i++)
                Rx[i] = 0;
            EmacPsRecv(&EmacPsInstance, rx_buffers, num_packet);
            now_packet=0;
            if(net_poll_mode!=0)
            {
                while(now_packet<num_packet)
                {
                    now_packet=0;
                    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
                    do_block(&(current_running->list),&recv_queue);
                    do_scheduler();
                    
                    EmacPsWaitRecv(&EmacPsInstance, num_packet, rx_len);
                }
                //screen_move_cursor(30, 1);
                //prints("now:%d\n", now_packet);
            }
            else
                EmacPsWaitRecv(&EmacPsInstance, num_packet, rx_len);
            for(int i=0; i<num_packet; i++)
            {
                src=(unsigned char*)rx_buffers[i];
                for(int j=0;j<rx_len[i];j++)
                {
                    *dest = *src;
                    dest++;
                    src++;
                }
                *frLength = rx_len[i];
                frLength++;
                rx_len[i] = 0;
                //XEmacPs_BdClearRxNew(Rx+i);
            }
            num_packet=0;
            u32 Reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
                XEMACPS_RXSR_OFFSET);
            XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, 
                XEMACPS_RXSR_OFFSET, 
                Reg | XEMACPS_RXSR_FRAMERX_MASK);
        }
    }
    return 1;
}

void do_net_send(uintptr_t addr, size_t length)
{
    Tx[0] = 0;
    memcpy(tx_buffer, addr, length);
    EmacPsSend(&EmacPsInstance, tx_buffer, length);
    if(net_poll_mode!=0)
    {
        while(EmacPsWaitSend(&EmacPsInstance))
        {
            current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
            do_block(&(current_running->list),&send_queue);
            do_scheduler();
        }
    }
    else
        EmacPsWaitSend(&EmacPsInstance);
}

void do_net_irq_mode(int mode)
{
    u32 reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
        XEMACPS_TXSR_OFFSET);
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress,
        XEMACPS_TXSR_OFFSET, reg | XEMACPS_TXSR_TXCOMPL_MASK);
    
    u32 reg2 = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress,
        XEMACPS_RXSR_OFFSET);
    XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress,
        XEMACPS_RXSR_OFFSET, reg2 | XEMACPS_RXSR_FRAMERX_MASK);
    //screen_move_cursor(60, 1);
    //prints("mode:%d",mode);
    net_poll_mode = mode;
    if(mode==1||mode==4)
    {
        XEmacPs_IntEnable(&EmacPsInstance,
            (XEMACPS_IXR_FRAMERX_MASK | XEMACPS_IXR_TXCOMPL_MASK));
    }
    else
    {
        XEmacPs_IntDisable(&EmacPsInstance,
            (XEMACPS_IXR_FRAMERX_MASK | XEMACPS_IXR_TXCOMPL_MASK));
    }
    
    for(int i=0;i<32;i++)
        Rx[i] = 0;

}
