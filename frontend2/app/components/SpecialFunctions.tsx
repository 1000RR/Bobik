"use client";
import React, { MouseEventHandler } from "react";
import styled, {css} from "styled-components";
import Button from "./Button";
import { emitClearDataEvent, emitTestAlarmEvent, emitGetAttentionEvent, emitSendSpecialOnce, emitSendSpecialRepeatedly, emitStopSendingSpecial } from "@src/WebSocketService";
import Panel from "./Panel";
import { setPastEvents } from "./AppStateSlice";
import { useDispatch } from "react-redux";

export const PanelSizeStyle = css`
    width: 100%;
    height: content;
`;

export const PanelLayoutStyle = css`
    display: flex;
    justify-content: center;
    align-items: start;
    justify-content: space-between;
    gap: 10px;
    flex-direction: row;
    flex-wrap: wrap;
    padding: 10px 10px 10px 10px;
    border-radius: 5px;
`;

const PanelColorStyle = css`
    background-color: rgba(60,60,60, .3);
`;

const CompositePanelStyle = styled.div`
    ${PanelSizeStyle}
    ${PanelLayoutStyle}
    ${PanelColorStyle}
`;

const testAlarmsHandler : MouseEventHandler<HTMLButtonElement> = function(event) {
    emitTestAlarmEvent();
};

const getAttentionHandler : MouseEventHandler<HTMLButtonElement> = function(event) {
    emitGetAttentionEvent();
};
const sendSingleCan : MouseEventHandler<HTMLButtonElement> = function(event) {
    const message = 
        (document?.getElementById('sender-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('receiver-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('message-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('type-field') as HTMLInputElement)?.value;
    emitSendSpecialOnce(message);
};
const sendRepeatedlyCan : MouseEventHandler<HTMLButtonElement> = function(event) {
    const message = 
        (document?.getElementById('sender-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('receiver-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('message-field') as HTMLInputElement)?.value + ':' +
        (document?.getElementById('type-field') as HTMLInputElement)?.value;
    emitSendSpecialRepeatedly(message);
};
const stopSendingCan : MouseEventHandler<HTMLButtonElement> = function(event) {
    emitStopSendingSpecial();
};

const SpecialFunctions: React.FC<{
    className?: string
}> = ({ className }) => {
    const clearDataHandler : MouseEventHandler<HTMLButtonElement> = function(event) {
        dispatch(setPastEvents({ //clear past events in model immediately (UI will reflect this)
            pastEvents: []
        }));
        emitClearDataEvent();
    };

    const dispatch = useDispatch();

    return (
        <CompositePanelStyle className={className}>
            <Button className="blueButton dimmable" onClick={clearDataHandler}>Clear Data</Button>
            <Button className="orangeButton dimmable" onClick={testAlarmsHandler}>Test Alarms (current profile-selected)</Button>
            <Button className="redButton dimmable" onClick={getAttentionHandler}>Get Attention</Button>
            <Panel>
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
            <Button id="can-send-single" className="smallbutton gray dimmable" onClick={sendSingleCan}>send once</Button>
            <Button id="can-send-repeatedly" className="smallbutton gray dimmable" onClick={sendRepeatedlyCan}>send repeatedly</Button>
            <Button id="can-stop-send" className="smallbutton gray dimmable" onClick={stopSendingCan}>stop sending</Button>
            </Panel>
        </CompositePanelStyle>
    );
};

export default SpecialFunctions;
 
 
