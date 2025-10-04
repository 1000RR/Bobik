"use client";
import React from "react";
import styled, {css} from "styled-components";

export type CSSAlignItems =
    | 'flex-start' | 'flex-end' | 'center' | 'baseline'
    | 'stretch' | 'start' | 'end' | 'self-start' | 'self-end'
    | 'normal' | 'inherit' | 'initial' | 'unset';

export type CSSPosition = 
    | "absolute" | "relative" | "fixed" | "sticky";

export type CSSJustifyContent =
    | "flex-start" | "flex-end" | "center" | "space-between"
    | "space-around" | "space-evenly" | "start" | "end"
    | "left" | "right" | "normal";

export const PanelSizeStyle = css`
    width: 100%;
    height: content;
    min-height: 100px;
`;

export const PanelLayoutStyle = css`
    display: flex;
    align-items: center;
    justify-content: space-evenly;
    gap: 10px;
    row-gap: 20px;
    flex-direction: row;
    padding: 20px 0px 20px 0px;
    flex-wrap: wrap;
    position: relative;
`;

export const PanelBorderStyle = css`
    border-radius: 5px;
`;

const PanelBackgroundColorStyle = css`
    background-color: rgba(60,60,60, .5);
`;

const CompositeStyledPanel = styled.div<{hidebackground?: boolean}>`
    ${PanelSizeStyle}
    ${PanelLayoutStyle}
    ${({ hidebackground }) => hidebackground ? '' : PanelBackgroundColorStyle}
    ${PanelBorderStyle}
`;

const Panel: React.FC<{
    className?: string,
    children?: React.ReactNode,
    flexDirection?: 'row' | 'column',
    gap?: string,
    rowGap?: string,
    alignItems?: CSSAlignItems,
    margin?: string,
    padding?: string
    hidebackground?: boolean,
    justifyContent?: CSSJustifyContent,
    zIndex?: number,
    position?: CSSPosition,
    width?: string,
    height?: string,
    minHeight?: string
}> = ({ className, children, flexDirection, gap, rowGap, alignItems, margin, padding, hidebackground, justifyContent, zIndex, position, width, height, minHeight }) => {
    
    return (
        <CompositeStyledPanel hidebackground={hidebackground} className={className} style={{flexDirection:flexDirection, gap:gap, rowGap: rowGap, alignItems: alignItems, margin: margin, padding: padding, justifyContent: justifyContent, zIndex: zIndex, position: position, width: width, height: height, minHeight: minHeight}}>
           {children}
        </CompositeStyledPanel>
    );
};

export default Panel;