/* eslint-disable @typescript-eslint/no-unused-expressions */
"use client";
import Image from "next/image";
import React from "react";
import { useSelector } from "react-redux";
import styled, {css} from "styled-components";
//import { emitDisarmEvent, emitArmEvent } from "@src/WebSocketService";
import { AppStateSlice } from "./AppStateSlice";
import Clock from "@components/Clock";
import PowerIndicator from "@components/PowerIndicator";

const ButtonSizeStyle = css`
    width: 100%;
    height: 100px;
    font-size: 2em;

    @media only screen and (min-device-width: 320px) and (max-device-width: 430px) and (-webkit-device-pixel-ratio: 2), 
       only screen and (min-device-width: 375px) and (max-device-width: 812px) and (-webkit-device-pixel-ratio: 3)
    {
       
       height: 100px;
       font-size: 1.5em;
    }
`;
const ButtonBorderStyle = css`
    border: 2px;
    border-style: solid;
    border-radius: 5px;
    border-color: darkgrey;
`;
const ButtonLayoutStyle = css`
    display: flex;
    justify-content: center;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    flex-direction: row;
    padding: 10px;
`;

const ButtonTextStyle = css`
    font-family: "futura";
    font-size: 25px;
`;



const CompositeStyledButton = styled.button.attrs({
  className: "noselect"
})`
  ${ButtonSizeStyle}
  ${ButtonBorderStyle}
  ${ButtonLayoutStyle}
  ${ButtonTextStyle}
`;

const AlarmEnableButton: React.FC<{
    className?: string,
    children?: React.ReactNode,
    showIcon?: boolean
}> = ({ className, children, showIcon}) => {
    const imgSrcArmed = "/assets/attackdog.jpg";
    const imgSrcDisarmed = "/assets/dogue.jpg";

    const isArmed = useSelector(function (state: AppStateSlice) { 
        return state.appState.status?.armStatus === "ARMED"
    });

    const profileName = useSelector(function (state: AppStateSlice) { 
        return state.appState.status?.profile
    });

    // const handler:React.MouseEventHandler<HTMLButtonElement> = function(event) {
    //     isArmed ? emitDisarmEvent() : emitArmEvent();
    // };

    const buttonText = isArmed ? `ARMED : ${profileName.toUpperCase()}` : "DISARMED";
    // const TapText = styled.div`
    //     font-size: .75em;
    // `;

    const alarmTriggered = useSelector(function (state: AppStateSlice) { 
        return state.appState.status.alarmStatus === 'ALARM';
    });

    return (<div className={"line_height_reduced"} style={{ width: "100%" }}>
        <CompositeStyledButton className={`alarm-state-button ${className} ${isArmed ? 'alarm-state-on' : 'alarm-state-off'}`} onClick={(e)=>{e.currentTarget.blur(); /*handler(e);*/}}>
            {showIcon ? <Image className={`fadeoutImageRound scale_mobile ${alarmTriggered ? 'invertTransitions' : ''}`} alt="" height="90" width="90" src={isArmed ? imgSrcArmed : imgSrcDisarmed}></Image> : <PowerIndicator secondsPerRotation={isArmed ? 1 : 0} color={isArmed ? "cyan" : "maroon"} dotColor={isArmed ? "#215dbe" : "red"}></PowerIndicator>}
                <Clock></Clock>
                <div className={"alarm-profile"}>
                    {buttonText}
		    {/*<TapText>quick tap to toggle</TapText>*/}
                </div>
                {children}
        </CompositeStyledButton>
        </div>
    );
};

export default AlarmEnableButton;
