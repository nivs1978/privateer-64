param(
    [string]$Source = "grid.csv",
    [string]$Out = "grid.dta"
)

$values = New-Object System.Collections.Generic.List[byte]
foreach ($line in Get-Content $Source) {
    foreach ($part in ($line -split ',')) {
        $trimmed = $part.Trim()
        if ($trimmed -eq '') {
            continue
        }

        $value = [int]$trimmed
        if ($value -lt 0 -or $value -gt 255) {
            throw "Grid value out of byte range: $value"
        }

        $values.Add([byte]$value)
    }
}

if ($values.Count -ne 406) {
    throw "Expected 406 grid bytes, got $($values.Count)"
}

$bytes = New-Object byte[] 408
$bytes[0] = 192
$bytes[1] = 53
for ($i = 0; $i -lt $values.Count; $i++) {
    $bytes[$i + 2] = $values[$i]
}

[System.IO.File]::WriteAllBytes($Out, $bytes)