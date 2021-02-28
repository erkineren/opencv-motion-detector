<?php


$_FILES = normalize_files_array($_FILES);

$count = count($_FILES['fileToUpload']);

$uploaddir = 'uploads/';
if ( ! is_dir($uploaddir)) {
    mkdir($uploaddir);
}

for( $i = 0; $i < $count; $i++ ){
	
	$uploadfile = $uploaddir . basename($_FILES['fileToUpload'][$i]['name']);
	
	if (move_uploaded_file($_FILES['fileToUpload'][$i]['tmp_name'], $uploadfile)) {
		//echo "File is valid, and was successfully uploaded.\n";
	} else {
		echo "Possible file upload attack!\n";
	}
	
	//echo 'Here is some more debugging info:<br>';
	//print_r($_FILES['fileToUpload'][$i]);
	
}


function normalize_files_array($files = []) {

	$normalized_array = [];

	foreach($files as $index => $file) {

		if (!is_array($file['name'])) {
			$normalized_array[$index][] = $file;
			continue;
		}

		foreach($file['name'] as $idx => $name) {
			$normalized_array[$index][$idx] = [
				'name' => $name,
				'type' => $file['type'][$idx],
				'tmp_name' => $file['tmp_name'][$idx],
				'error' => $file['error'][$idx],
				'size' => $file['size'][$idx]
			];
		}

	}

	return $normalized_array;

}
?>