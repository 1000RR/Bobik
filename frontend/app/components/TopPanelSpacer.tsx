"use client";
import React from "react";
import Panel from "@components/Panel";
import { useScrollThreshold } from "@components/useScrollThreshold";

const TopPanelSpacer: React.FC<{
    className?: string
}> = ({ className }) => {

    const shrink = useScrollThreshold(10); //scrolled past 10px
    
    return (
        <Panel hidebackground={true} minHeight={"0px"} height={shrink ? "0" : "120px"} className={"topPanel " + className}></Panel>
    );
};

export default TopPanelSpacer;