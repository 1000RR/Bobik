/* eslint-disable @typescript-eslint/no-unused-vars */
/* eslint-disable no-unused-vars */
"use client";
import React, {useState} from "react";
import styled, {css} from "styled-components";

const drawerDisplay = 'flex';

const ButtonSizeStyle = css`
	width: calc(100% - 20px);
	height: 65px;

	@media only screen and (min-device-width: 320px) and (max-device-width: 430px) and (-webkit-device-pixel-ratio: 2), 
	   only screen and (min-device-width: 375px) and (max-device-width: 812px) and (-webkit-device-pixel-ratio: 3)
	{
	   font-size: 1.5em;
	}
`;

const ButtonTextStyle = css`
	text-align: left;
	font-size: 23px;
	font-weight: normal;
	font-family: "futura";
`;

const ButtonBorderStyle = css`
	border: 1px;
	border-style: solid;
	border-radius: 5px;
	margin: 10px 10px 0px 10px;
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

const CompositeStyledButton = styled.button.attrs({
  className: "noselect"
})`
  ${ButtonSizeStyle}
  ${ButtonBorderStyle}
  ${ButtonLayoutStyle}
  ${ButtonTextStyle}
`;

const DrawerSpacing = css`
	gap: 10px;
	padding: 10px;
`;

const Drawer = styled.div<{
	disableinternalspacing?: boolean
}>`
	width: calc(100% - 20px);
	background-color: rgba(55, 55, 55, .4);
	margin: 0px 10px 5px 10px;
	border-radius: 5px;
	display: ${drawerDisplay};
	position: relative;
	justify-content: space-around; 
	overflow-y: auto;
	overflow-x: hidden;
	height: auto;
	
	${({ disableinternalspacing }) => !disableinternalspacing && DrawerSpacing}
`;

const ShrinkContainingDiv = styled.div.withConfig({
  shouldForwardProp: (prop) => prop !== "iscollapsed",
})<{ iscollapsed: boolean }>`
	display: flex;
	flex-direction: column;
	justify-content: center;
	align-items: center;
	height: ${({iscollapsed}) => iscollapsed ? '0px' : 'auto'};
	overflow: hidden;
	width: 100%;
`;

const ButtonWithDrawer: React.FC<{
	flexDirection: ('row' | 'column'),
	className?: string,
	children?: React.ReactNode,
	buttonText?: string,
	justifyContent? : ('space-around' | 'space-between' | 'center' | 'flex-start' | 'flex-end'),
	onClick?: React.MouseEventHandler,
	containsScrollable?: boolean,
	disableinternalspacing?: boolean,
	isOpen?: boolean,
	keepChildrenInDomOnClose?: boolean
}> = ({ flexDirection, className, children, buttonText, justifyContent, containsScrollable, disableinternalspacing = false, isOpen = false, keepChildrenInDomOnClose=false}) => {
	
	const [isCollapsed, setIsCollapsed] = useState(!isOpen);
	const handler:React.MouseEventHandler<HTMLButtonElement> = function(event) {
		event.currentTarget.blur();
		setIsCollapsed(!isCollapsed);
	};

	return (
	<>
		<CompositeStyledButton className={`drawer-button ${className ? className : ''} ${isCollapsed ? '' : 'button-enabled'}`} onClick={handler}>
			{buttonText}
		</CompositeStyledButton>
		<Drawer style={{flexDirection: flexDirection, display: isCollapsed ? 'none' : 'flex', justifyContent: justifyContent, maxHeight: containsScrollable ? 'calc(100vh - 150px)': '' }} disableinternalspacing={disableinternalspacing}>
			{!keepChildrenInDomOnClose ? (!isCollapsed && children) : (<ShrinkContainingDiv iscollapsed={isCollapsed}>{children}</ShrinkContainingDiv>)}
		</Drawer>
	</>
	);
};

export default ButtonWithDrawer;