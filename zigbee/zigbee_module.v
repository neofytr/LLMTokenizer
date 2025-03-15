module zigbee_communication (
    input clk,                      
    input reset_n,                  
    input [7:0] sw,                
    input btn_send,                 
    output [7:0] led,               
    output led_rx,                  
    output led_tx,                  
    output led_error,               
    
    input uart_rx,                  // UART RX from Zigbee module
    output uart_tx                  // UART TX to Zigbee module
);

parameter CLOCK_FREQ = 100_000_000; // 100Mhz clock hi hoga
parameter BAUD_RATE = 9600;         // this is the common baudrate for AT communication
parameter UART_DIVIDER = CLOCK_FREQ / BAUD_RATE;

localparam IDLE = 4'd0;
localparam INIT_ZIGBEE_START = 4'd1;
localparam INIT_ZIGBEE_WAIT = 4'd2;
localparam INIT_ZIGBEE_NEXT = 4'd3;
localparam WAIT_FOR_SEND = 4'd4;
localparam SENDING = 4'd5;
localparam RECEIVING = 4'd6;
localparam ERROR = 4'd7;

localparam AT_COMMAND_COUNT = 6;
reg [7:0] at_commands[0:AT_COMMAND_COUNT-1][0:9]; 
reg [3:0] cmd_index = 0;
reg [3:0] char_index = 0;
reg [3:0] cmd_length[0:AT_COMMAND_COUNT-1];
reg [31:0] delay_counter = 0;

reg btn_send_prev = 0;
reg btn_send_debounce = 0;
reg [19:0] debounce_counter = 0;
wire btn_send_posedge;

reg [3:0] state = IDLE;
reg [7:0] data_to_send;
reg transmit_valid = 0;
wire transmit_active;
wire transmit_done;
wire receive_valid;
wire [7:0] received_byte;
reg data_received = 0;
reg [7:0] rx_data = 0;
reg transmit_complete = 0;
reg init_complete = 0;
reg error_flag = 0;

transmitter #(
    .FREQUENCY(UART_DIVIDER)
) uart_tx_inst (
    .clk(clk),
    .i_Data_Valid(transmit_valid),
    .i_Byte(data_to_send),
    .o_Transmit_Active(transmit_active),
    .o_Serial_Output(uart_tx),
    .o_Transmit_Done(transmit_done)
);

receiver #(
    .FREQUENCY(UART_DIVIDER)
) uart_rx_inst (
    .clk(clk),
    .i_Serial_Data(uart_rx),
    .o_Data_Valid(receive_valid),
    .o_Received_Byte(received_byte)
);

initial begin
    // "+++" (Enter AT command mode)
    at_commands[0][0] = "+"; at_commands[0][1] = "+"; at_commands[0][2] = "+";
    cmd_length[0] = 3;
    
    // "ATID1234" (Set PAN ID)
    at_commands[1][0] = "A"; at_commands[1][1] = "T"; at_commands[1][2] = "I"; at_commands[1][3] = "D";
    at_commands[1][4] = "1"; at_commands[1][5] = "2"; at_commands[1][6] = "3"; at_commands[1][7] = "4";
    at_commands[1][8] = 13; // CR
    cmd_length[1] = 9;
    
    // "ATMY1" (Set 16-bit source address)
    at_commands[2][0] = "A"; at_commands[2][1] = "T"; at_commands[2][2] = "M"; at_commands[2][3] = "Y";
    at_commands[2][4] = "1"; at_commands[2][5] = 13; // CR
    cmd_length[2] = 6;
    
    // "ATDH0" (Set destination address high)
    at_commands[3][0] = "A"; at_commands[3][1] = "T"; at_commands[3][2] = "D"; at_commands[3][3] = "H";
    at_commands[3][4] = "0"; at_commands[3][5] = 13; // CR
    cmd_length[3] = 6;
    
    // "ATDL2" (Set destination address low)
    at_commands[4][0] = "A"; at_commands[4][1] = "T"; at_commands[4][2] = "D"; at_commands[4][3] = "L";
    at_commands[4][4] = "2"; at_commands[4][5] = 13; // CR
    cmd_length[4] = 6;
    
    // "ATCN" (Exit command mode)
    at_commands[5][0] = "A"; at_commands[5][1] = "T"; at_commands[5][2] = "C"; at_commands[5][3] = "N";
    at_commands[5][4] = 13; // CR
    cmd_length[5] = 5;
end

always @(posedge clk) begin
    if (!reset_n) begin
        btn_send_prev <= 0;
        btn_send_debounce <= 0;
        debounce_counter <= 0;
    end else begin
        btn_send_prev <= btn_send_debounce;
        
        if (btn_send != btn_send_debounce) begin
            debounce_counter <= debounce_counter + 1;
            if (debounce_counter == 20'hFFFFF) begin
                btn_send_debounce <= btn_send;
                debounce_counter <= 0;
            end
        end else begin
            debounce_counter <= 0;
        end
    end
end

assign btn_send_posedge = btn_send_debounce && !btn_send_prev;

always @(posedge clk or negedge reset_n) begin
    if (!reset_n) begin
        state <= IDLE;
        transmit_valid <= 0;
        data_received <= 0;
        cmd_index <= 0;
        char_index <= 0;
        delay_counter <= 0;
        error_flag <= 0;
        init_complete <= 0;
    end else begin
        case (state)
            IDLE: begin
                // Reset signals
                transmit_valid <= 0;
                data_received <= 0;
                cmd_index <= 0;
                char_index <= 0;
                delay_counter <= 0;
                error_flag <= 0;
                
                state <= INIT_ZIGBEE_START;
            end
            
            INIT_ZIGBEE_START: begin
                data_to_send <= at_commands[cmd_index][char_index];
                transmit_valid <= 1;
                state <= INIT_ZIGBEE_WAIT;
            end
            
            INIT_ZIGBEE_WAIT: begin
                if (transmit_active)
                    transmit_valid <= 0;
                    
                if (transmit_done) begin
                    if (delay_counter < CLOCK_FREQ) begin
                        delay_counter <= delay_counter + 1;
                    end else begin
                        delay_counter <= 0;
                        state <= INIT_ZIGBEE_NEXT;
                    end
                end
            end
            
            INIT_ZIGBEE_NEXT: begin
                if (char_index < cmd_length[cmd_index] - 1) begin
                    char_index <= char_index + 1;
                    state <= INIT_ZIGBEE_START;
                end else begin
                    char_index <= 0;
                    
                    if (cmd_index == 0) begin
                        if (delay_counter < CLOCK_FREQ) begin
                            delay_counter <= delay_counter + 1;
                        end else begin
                            delay_counter <= 0;
                            cmd_index <= cmd_index + 1;
                            state <= INIT_ZIGBEE_START;
                        end
                    else begin
                        if (cmd_index < AT_COMMAND_COUNT - 1) begin
                            cmd_index <= cmd_index + 1;
                            state <= INIT_ZIGBEE_START;
                        end else begin
                            init_complete <= 1;
                            state <= WAIT_FOR_SEND;
                        end
                    end
                end
            end
            
            WAIT_FOR_SEND: begin
                transmit_complete <= 0;
                
                if (btn_send_posedge) begin
                    data_to_send <= sw;  
                    transmit_valid <= 1;
                    state <= SENDING;
                end
                
                if (receive_valid) begin
                    rx_data <= received_byte;
                    data_received <= 1;
                    state <= RECEIVING;
                end
            end
            
            SENDING: begin
                if (transmit_active)
                    transmit_valid <= 0;
                    
                if (transmit_done) begin
                    transmit_complete <= 1;
                    state <= WAIT_FOR_SEND;
                end
            end
            
            RECEIVING: begin
                if (delay_counter < CLOCK_FREQ/2) begin  
                    delay_counter <= delay_counter + 1;
                end else begin
                    delay_counter <= 0;
                    data_received <= 0;
                    state <= WAIT_FOR_SEND;
                end
            end
            
            ERROR: begin
                error_flag <= 1;
                if (delay_counter < CLOCK_FREQ*2) begin 
                    delay_counter <= delay_counter + 1;
                end else begin
                    delay_counter <= 0;
                    error_flag <= 0;
                    state <= IDLE;  // Restart from IDLE
                end
            end
            
            default:
                state <= IDLE;
        endcase
    end
end

assign led = rx_data;  
assign led_rx = data_received;
assign led_tx = transmit_complete;
assign led_error = error_flag;

endmodule