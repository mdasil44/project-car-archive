//written with androidscript

//Vars
var stop = 0;
var str  = 0.5;
var spd  = 0.5;
var record = false;
const slow = 0.5;
const fast = 2.0;

//Called when application is started.
function OnStart()
{
	//Prevent screen from dimming or locking while using app
	app.PreventScreenLock( true );

	//Create a layout with objects vertically centered.
	lay = app.CreateLayout( "linear", "VCenter,FillXY" );

	//Create a button 1/3 of screen width and 1/4 screen height.
	btn = app.CreateButton( "Connect", 0.4, 0.15 );
	btn.SetOnTouch( btn_Connect_OnTouch );
	lay.AddChild( btn );

	//Create stop button
	tgls = app.CreateButton( "STOP!", 0.9, 0.45, "Lego");
	tgls.SetOnTouch( tgl_OnTouch );
	lay.AddChild( tgls );

	//Create recording toggle button
	tglRec = app.CreateButton( "Record", 0.4, 0.15, "Lego");
	tglRec.SetOnTouch( tgl_Rec_OnTouch );
	lay.AddChild( tglRec );

	//Create speed toggle button
	btnSpeed = app.CreateButton( "Toggle Speed" );
	btnSpeed.SetOnTouch( btn_Speed_OnTouch );
	lay.AddChild( btnSpeed );

	//Steering slider
	skb = app.CreateSeekBar( 0.8 );
	skb.SetRange( 1.0 );
	skb.SetValue( 0.5 );
	skb.SetOnTouch( skb_OnTouch );
	lay.addAddChild( skb );

	//Create layout
	app.AddLayout( lay );

	//Create Bluetooth serial object.
	bt = app.CreateBluetoothSerial();
	bt.SetOnConnect( bt_OnConnect );
	bt.SetOnReceive( bt_OnReceive );
	// bt.Connect( "SerialBT" );
	bt.SetSplitMode( "End", "\n" );


}

//Called when user touches the button.
function btn_Connect_OnTouch()
{
    bt.Connect( "SerialBT" );
}

//Called when toggle button is touched
function btn_Speed_OnTouch()
{
	if (spd == slow) speed = fast;
	else spd = slow;
}

//Called when we are connected.
function bt_OnConnect( ok )
{
	if ( ok ) setTimeout( communicate, 100 );
	else app.ShowPopup( "Failed to connect!" );
}

//Called when user touches our toggle button.
function tgl_OnTouch()
{
	if(stop)
	{
		stop = 0;
		tgls.SetText( "STOP!" );
	}
	else
	{
		stop = 1;
		tgls.SetText( "GO!" );
	}
}

//Called when the user touches the toggle recoring button
function tgl_Rec_OnTouch()
{
	if (record)
		record = false;
	else
		record = true;
}

//slider touch function
function skb_OnTouch( value )
{
	str = value;
}

//timed comunication function
function communicate()
{
	bt.Write( stop + "," + spd + "," + str + "," + record + "\n");
	//bt.write
	setTimeout( communicate, 100 );
}
