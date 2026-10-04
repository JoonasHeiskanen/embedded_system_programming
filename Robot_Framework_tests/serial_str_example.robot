*** Settings ***
Library   String
Library   SerialLibrary

*** Variables ***
${com}   	COM9
${baud} 	115200
${board}	nRF
${seq}      0RYGX
${error}    -1X

# Correct time variables
${correct_time}		000005X
${expected_time}	5X
${correct_time2}	000115X
${expected_time2}	75X

# Incorrect time variables
${incorrect_time}	000061X
${expected_error}	-3X
${incorrect_time2}	006000X

# Incorrect string length variables
${incorrect_str}		0235X
${expected_len_error}	-1X


*** Test Cases ***
Connect Serial
	Log To Console  Connecting to ${board}
	Add Port  ${com}  baudrate=${baud}  encoding=ascii
	Port Should Be Open  ${com}
	Reset Input Buffer
	Reset Output Buffer

Serial Led Control
	# lähetetään RYG ja lopetusmerkki X
	Write Data   ${seq}   encoding=ascii 
	Log To Console   Send sequence ${seq}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${error}
	Log To Console   Tested ${read} is same as ${error}

	# astetta hankalampi tehdä testaus numeroina
	# koska lopetusmerkki X pitää ensin poistaa merkkijonosta
	# tai vaihtaa lopetusmerkki esim \0
	# Should Be Equal As Integers   ${read}    -1
	
Send First Correct Time
	Reset Input Buffer
	# lähetetään aika
	Write Data	${correct_time}		encoding=ascii
	Log To Console	Send time ${correct_time}
	
	# vastaanotetaan merkkijono kunnes lopetusmerkki X
	${read} =	Read Until	terminator=58	encoding=ascii
	
	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console	 Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${expected_time}
	Log To Console   Tested ${read} is same as ${expected_time}
	
Send Second Correct Time
	Reset Input Buffer
	# lähetetään toinen aika
	Write Data	${correct_time2}		encoding=ascii
	Log To Console	Send time ${correct_time2}
	
	# vastaanotetaan merkkijono kunnes lopetusmerkki X
	${read} =	Read Until	terminator=58	encoding=ascii
	
	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console	 Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${expected_time2}
	Log To Console   Tested ${read} is same as ${expected_time2}
	
Send Incorrect Time
	Reset Input Buffer
	# lähetetään väärä aika
	Write Data 	${incorrect_time}	encoding=ascii
	Log To Console 	Send time ${incorrect_time}
	
	${read} =	Read Until	terminator=58	encoding=ascii
	Log To Console	 Received ${read}
	
	Should Be Equal As Strings	${read}		${expected_error}
	Log To Console	 Tested ${read} is same as ${expected_error}
	
Send Second Incorrect Time
	Reset Input Buffer
	# lähetetään väärä aika
	Write Data 	${incorrect_time2}	encoding=ascii
	Log To Console 	Send time ${incorrect_time2}
	
	${read} =	Read Until	terminator=58	encoding=ascii
	Log To Console	 Received ${read}
	
	Should Be Equal As Strings	${read}		${expected_error}
	Log To Console	 Tested ${read} is same as ${expected_error}
	
Send Incorrect String Length
	Reset Input Buffer
	# lähetetään väärän mittainen merkkijono
	Write Data	${incorrect_str}	encoding=ascii
	Log To Console	Send string ${incorrect_str}
	
	${read} =	Read Until	terminator=58	encoding=ascii
	Log To Console	Received ${read}
	
	Should Be Equal As Strings	${read}		${expected_len_error}
	Log To Console	Tested ${read} is same as ${expected_len_error}
	
	
Disconnect Serial
	Log To Console  Disconnecting ${board}
	[TearDown]  Delete Port  ${com}


	
	
	

