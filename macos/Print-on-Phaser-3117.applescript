-- SPDX-License-Identifier: GPL-2.0-only
-- macOS PDF Services sends the generated PDF using an open event.
-- No application control, network access, or UI scripting is required.
on open pdfFiles
    repeat with pdfFile in pdfFiles
        set pdfPath to POSIX path of pdfFile
        try
            do shell script "/Library/Printers/Phaser3117Local/print-pdf.sh " & quoted form of pdfPath
        on error errorText number errorNumber
            display alert "Phaser 3117: printing failed" message errorText as critical
            return
        end try
    end repeat
end open
