"use client";
import React, { MouseEventHandler } from "react";
import Button from "./Button";
import { emitClearDataEvent, emitTestAlarmEvent, emitGetAttentionEvent, emitSendSpecialOnce, emitSendSpecialRepeatedly, emitStopSendingSpecial } from "@src/WebSocketService";
import Panel from "./Panel";
import { setPastEvents } from "./AppStateSlice";
import { useDispatch } from "react-redux";

const testAlarmsHandler : MouseEventHandler<HTMLButtonElement> = function() {
    emitTestAlarmEvent();
};

const getAttentionHandler : MouseEventHandler<HTMLButtonElement> = function() {
    emitGetAttentionEvent();
};
const sendSingleCan : MouseEventHandler<HTMLButtonElement> = function() {
    const message = 
        (document?.getElementById('sender-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('receiver-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('message-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('type-field') as HTMLInputElement)?.value;
    emitSendSpecialOnce(message);
};
const sendRepeatedlyCan : MouseEventHandler<HTMLButtonElement> = function() {
    const message = 
        (document?.getElementById('sender-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('receiver-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('message-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('type-field') as HTMLInputElement)?.value;
    emitSendSpecialRepeatedly(message);
};
const stopSendingCan : MouseEventHandler<HTMLButtonElement> = function() {
    emitStopSendingSpecial();
};

const SpecialFunctions: React.FC<{
    className?: string
}> = ({ className }) => {
    const clearDataHandler : MouseEventHandler<HTMLButtonElement> = function() {
        dispatch(setPastEvents({ //clear past events in model immediately (UI will reflect this)
            pastEvents: []
        }));
        emitClearDataEvent();
    };

    const dispatch = useDispatch();

    return (
        <>
        <Panel padding="10px" hideBackground={true} className={className}>
            <Button className="mediumbutton blueButton dimmable" onClick={clearDataHandler}>Clear Data</Button>
            <Button className="mediumbutton orangeButton dimmable" onClick={testAlarmsHandler}>Test Alarms (current profile-selected)</Button>
            <Button className="mediumbutton redButton dimmable" onClick={getAttentionHandler}>Get Attention</Button>
        </Panel>
        <Panel hideBackground={true} padding="10px">
            <div style={{width: "100%", display: "flex", justifyContent: "center", "fontSize": "1.2em"}}>Simulate incoming CAN device message</div>
            <div className="input-wrapper" style={{position: "relative", display: "inline-block"}}>
                <input type="text" id="sender-field" className="dimmable input-field" name="sender" defaultValue="0x75" required />
                <span className="input-hint" style={{position: "absolute", right: 7}}>FROM</span>
            </div>
            <div className="input-wrapper" style={{position: "relative", display: "inline-block"}}>
                <input type="text" id="receiver-field" className="dimmable input-field" name="receiver" defaultValue="0x14" required />
                <span className="input-hint" style={{position: "absolute", right: 7}}>TO</span>
            </div>
            <div className="input-wrapper" style={{position: "relative", display: "inline-block"}}>
                <input type="text" id="message-field" className="dimmable input-field" name="message" defaultValue="0xAA" required />
                <span className="input-hint" style={{position: "absolute", right: 7}}>MSG</span>
            </div>
            <div className="input-wrapper" style={{position: "relative", display: "inline-block"}}>
                <input type="text" id="type-field" className="dimmable input-field" name="type" defaultValue="0x00" required />
                <span className="input-hint" style={{position: "absolute", right: 7}}>TYPE</span>
            </div>
            <Panel justifyContent={"space-between"} padding="10px" hideBackground={true} className={className}>
                <Button id="can-send-single" className="smallbutton gray dimmable" onClick={sendSingleCan}>send 1x</Button>
                <Button id="can-send-repeatedly" className="smallbutton gray dimmable" onClick={sendRepeatedlyCan}>send Nx</Button>
                <Button id="can-stop-send" className="smallbutton gray dimmable" onClick={stopSendingCan}>stop</Button>
            </Panel>
        </Panel>
        </>
    );
};

export default SpecialFunctions;
 
 
