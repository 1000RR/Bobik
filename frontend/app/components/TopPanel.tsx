"use client";
import React, {useState, useEffect} from "react";
import styled, {css} from "styled-components";
import AlarmEnableButton from "@components/AlarmEnableButton"

export const PanelSizeStyle = css`
    width: 100%;
    min-width: 100%;
    height: 120px;

    @media only screen and (min-device-width: 320px) and (max-device-width: 430px) and (-webkit-device-pixel-ratio: 2), 
		only screen and (min-device-width: 375px) and (max-device-width: 812px) and (-webkit-device-pixel-ratio: 3) {
        height: 120px;
    }
`;

export const PanelLayoutStyle = css`
    display: flex;
    position: fixed;
    justify-content: center;
    align-items: start;
    justify-content: space-between;
    gap: 10px;
    flex-direction: column;
    padding: 10px 10px 10px 10px;
    border-radius: 5px;
    z-index:10;
`;

const PanelColorStyle = css`
    background-color: rgba(60,60,60, .7);
`;

const CompositeStyledPanel = styled.div`
    ${PanelSizeStyle}
    ${PanelLayoutStyle}
    ${PanelColorStyle}
`;

const TopPanel: React.FC = ({}) => {
    const [shrink, setShrink] = useState(false);

    useEffect(() => {
        const handleScroll = () => {
        setShrink(window.scrollY > 10);
    };

    window.addEventListener("scroll", handleScroll);
        return () => window.removeEventListener("scroll", handleScroll);
    }, []);

    return (
        <CompositeStyledPanel className={`topPanel ${shrink ? "shrink" : ""}`}>
            <AlarmEnableButton className={`alarmButton ${shrink ? "shrink" : ""}`}></AlarmEnableButton>
        </CompositeStyledPanel>
    );
};

export default TopPanel;